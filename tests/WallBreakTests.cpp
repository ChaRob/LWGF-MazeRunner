#include "../src/game/GameSession.h"
#include "../src/game/GameFlow.h"
#include "../src/game/GameTuning.h"
#include "../src/game/stage/Stage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr std::array<CellPosition, 4> Directions{{
        { 0, 1 }, { 0, -1 }, { -1, 0 }, { 1, 0 }
    }};

    void Require(bool _ok, const char* _message)
    {
        if (!_ok)
        {
            throw std::runtime_error(_message);
        }
    }

    std::vector<CellPosition> FindPath(const Maze& _maze,
        CellPosition _from, CellPosition _to)
    {
        const int width = _maze.GetCellWidth();
        const int height = _maze.GetCellHeight();
        const auto index = [width](CellPosition _p) { return _p.y * width + _p.x; };
        std::vector<int> parents(static_cast<std::size_t>(width * height), -1);
        std::queue<CellPosition> pending;
        parents[static_cast<std::size_t>(index(_from))] = index(_from);
        pending.push(_from);
        while (!pending.empty())
        {
            const CellPosition current = pending.front();
            pending.pop();
            if (current == _to)
            {
                break;
            }
            for (const CellPosition direction : Directions)
            {
                const CellPosition next{ current.x + direction.x,
                    current.y + direction.y };
                if (!_maze.CanMove(current, next)
                    || parents[static_cast<std::size_t>(index(next))] != -1)
                {
                    continue;
                }
                parents[static_cast<std::size_t>(index(next))] = index(current);
                pending.push(next);
            }
        }

        std::vector<CellPosition> result;
        int current = index(_to);
        Require(parents[static_cast<std::size_t>(current)] != -1,
            "Target must be reachable on existing maze corridors");
        while (current != index(_from))
        {
            const int previous = parents[static_cast<std::size_t>(current)];
            result.push_back({ current % width - previous % width,
                current / width - previous / width });
            current = previous;
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    struct BreakTarget
    {
        CellPosition from;
        CellPosition direction;
    };

    BreakTarget FindInternalWall(const Maze& _maze)
    {
        for (int y = 0; y < _maze.GetCellHeight(); ++y)
        {
            for (int x = 0; x < _maze.GetCellWidth(); ++x)
            {
                for (CellPosition direction : Directions)
                {
                    const CellPosition next{ x + direction.x, y + direction.y };
                    if (_maze.IsCellInside(next)
                        && !_maze.CanMove({ x, y }, next))
                    {
                        return { { x, y }, direction };
                    }
                }
            }
        }
        throw std::runtime_error("Expected at least one closed internal wall");
    }

    void Walk(GameSession& _game, const std::vector<CellPosition>& _path)
    {
        for (const CellPosition direction : _path)
        {
            _game.TryMove(direction);
        }
    }

    void RequirePerimeterIntact(const Maze& _maze)
    {
        for (int y = 0; y < _maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < _maze.GetTileWidth(); ++x)
            {
                const bool border = x == 0 || y == 0
                    || x == _maze.GetTileWidth() - 1
                    || y == _maze.GetTileHeight() - 1;
                Require(_maze.IsOuterWall(x, y) == border,
                    "Outer wall classification must match tile perimeter");
                if (border)
                {
                    Require(_maze.GetTile(x, y) == TileType::Wall,
                        "No outer wall may be destroyed");
                }
            }
        }
        Require(!_maze.IsOuterWall(-1, 0)
            && !_maze.IsOuterWall(_maze.GetTileWidth(), 0),
            "Out-of-bounds tiles must not be considered drawable perimeter");
    }

    void TestSpawnAndPlacement()
    {
        using namespace GameTuning::WallBreak;
        Require(GetSpawnChance(1) == 0.05 && GetSpawnChance(30) == 0.05,
            "Stages 1-30 must have 5% Wall Break chance");
        Require(GetSpawnChance(31) == 0.075 && GetSpawnChance(40) == 0.075,
            "Stages 31-40 must have 7.5% Wall Break chance");
        Require(GetSpawnChance(41) == 0.10 && GetSpawnChance(100) == 0.10,
            "Stages 41+ must have 10% Wall Break chance");

        constexpr int SampleCount = 1200;
        for (const std::uint64_t number : { 1ull, 31ull, 41ull })
        {
            int appeared = 0;
            for (int i = 0; i < SampleCount; ++i)
            {
                const Stage stage(number, static_cast<std::uint32_t>(i));
                const Stage duplicate(number, static_cast<std::uint32_t>(i));
                Require(stage.GetWallBreakPosition() == duplicate.GetWallBreakPosition(),
                    "Same stage seed must reproduce Wall Break placement");
                if (!stage.GetWallBreakPosition())
                {
                    continue;
                }
                ++appeared;
                const CellPosition item = *stage.GetWallBreakPosition();
                Require(stage.GetMaze().IsCellInside(item)
                    && item != stage.GetEndpoints().start
                    && item != stage.GetEndpoints().goal,
                    "Wall Break must appear on a traversable non-endpoint Cell");
                Require(!stage.GetRecallPosition()
                    || item != *stage.GetRecallPosition(),
                    "Wall Break must not overlap Recall");
                Require(!stage.GetTimeOrbPosition()
                    || item != *stage.GetTimeOrbPosition(),
                    "Wall Break must not overlap Time Orb");
            }
            Require(std::fabs(static_cast<double>(appeared) / SampleCount
                - GetSpawnChance(number)) < 0.04,
                "Wall Break sampling must match developer-tuned chance");
        }
    }

    std::uint32_t FindSessionSeed()
    {
        for (std::uint32_t seed = 0; seed < 10000u; ++seed)
        {
            std::mt19937 rng(seed);
            const Stage stage(1, rng());
            if (stage.GetWallBreakPosition())
            {
                return seed;
            }
        }
        throw std::runtime_error("Unable to find seeded first-stage Wall Break");
    }

    void TestPermanentWallChangeAndFailedUses()
    {
        Stage stage(1, 100u);
        const Maze& maze = stage.GetMaze();
        const BreakTarget target = FindInternalWall(maze);
        const CellPosition destination{ target.from.x + target.direction.x,
            target.from.y + target.direction.y };
        const int shortestDistance = stage.GetEndpoints().shortestDistance;
        Require(!maze.CanMove(target.from, destination),
            "Chosen internal wall must initially block a step");
        RequirePerimeterIntact(maze);

        Require(!stage.TryBreakWall({ 0, 0 }),
            "A zero direction cannot destroy a wall");
        Require(!stage.TryBreakWall({ 2, 0 }),
            "Skipping a Cell cannot destroy a wall");
        Require(!stage.TryBreakWall({ 1, 1 }),
            "Diagonal direction cannot destroy a wall");

        // Direct Maze API tests for all forbidden boundary crossings and
        // mutation. Stage action is exercised separately through inventory.
        Maze mazeCopy = maze;
        Require(!mazeCopy.TryBreakWall({ 0, 0 }, { -1, 0 }),
            "Cannot break outside from leftmost logical Cell");
        Require(!mazeCopy.TryBreakWall({ 0, 0 }, { 0, -1 }),
            "Cannot break outside from bottom logical Cell");
        Require(!mazeCopy.TryBreakWall({ maze.GetCellWidth() - 1, 0 },
            { maze.GetCellWidth(), 0 }), "Cannot break outside from right edge");
        Require(!mazeCopy.TryBreakWall({ 0, maze.GetCellHeight() - 1 },
            { 0, maze.GetCellHeight() }), "Cannot break outside from upper edge");
        Require(!mazeCopy.TryBreakWall({ -1, 0 }, { 0, 0 }),
            "Outside position cannot be used to destroy inward wall");

        Require(mazeCopy.TryBreakWall(target.from, destination),
            "Closed internal wall should be permanently removed");
        Require(mazeCopy.CanMove(target.from, destination)
            && mazeCopy.CanMove(destination, target.from),
            "New passage must allow moving back and forth");
        Require(!mazeCopy.TryBreakWall(target.from, destination),
            "A previously opened corridor cannot be broken twice");
        RequirePerimeterIntact(mazeCopy);
        Require(stage.GetEndpoints().shortestDistance == shortestDistance,
            "Original stage distance remains the clear bonus reference");
    }

    void TestCollectCarryBreakAndRecall()
    {
        GameSession game(FindSessionSeed());
        const Stage& first = game.GetStage();
        const CellPosition start = first.GetEndpoints().start;
        const CellPosition collectible = *first.GetWallBreakPosition();
        const CellPosition goal = first.GetEndpoints().goal;
        Require(!game.TryUseItem(ActiveItemType::WallBreak, { 0, 1 }),
            "Without inventory a wall cannot be broken");
        const std::vector<CellPosition> collectPath =
            FindPath(first.GetMaze(), start, collectible);
        Walk(game, collectPath);
        Require(game.WasWallBreakCollectedOnLastMove()
            && !game.GetStage().GetWallBreakPosition()
            && game.GetInventory().GetCount(ActiveItemType::WallBreak) == 1,
            "Picking up Wall Break must add exactly one and remove item tile");

        const std::vector<CellPosition> finishPath =
            FindPath(game.GetStage().GetMaze(), collectible, goal);
        Walk(game, finishPath);
        Require(game.GetStage().GetNumber() == 2
            && game.GetCompletedStages() == 1
            && game.GetInventory().GetCount(ActiveItemType::WallBreak) == 1,
            "Wall Break inventory must persist across stage change");

        const Maze& secondMaze = game.GetStage().GetMaze();
        const CellPosition secondStart = game.GetStage().GetEndpoints().start;
        const BreakTarget target = FindInternalWall(secondMaze);
        Walk(game, FindPath(secondMaze, secondStart, target.from));
        const CellPosition player = game.GetStage().GetPlayer().GetPosition();
        Require(player == target.from, "Player should stand next to a closed wall");
        const CellPosition to{ player.x + target.direction.x,
            player.y + target.direction.y };
        const float beforeTime = game.GetRemainingTime();
        const int beforeDistance = game.GetStage().GetEndpoints().shortestDistance;

        Require(!game.TryUseItem(ActiveItemType::WallBreak, { 0, 0 })
            && !game.TryUseItem(ActiveItemType::WallBreak, { 1, 1 }),
            "Invalid directions must not consume the item");
        Require(!game.TryUseItem(ActiveItemType::WallBreak, { 99, 0 }),
            "Cannot break a distant or outer wall");
        Require(game.GetInventory().GetCount(ActiveItemType::WallBreak) == 1,
            "Failed Wall Break use must preserve inventory");

        Require(game.TryUseItem(ActiveItemType::WallBreak, target.direction),
            "Valid adjacent internal wall break must succeed");
        Require(game.GetInventory().GetCount(ActiveItemType::WallBreak) == 0
            && secondMaze.CanMove(player, to)
            && game.GetStage().GetPlayer().GetPosition() == player,
            "One item must be consumed, passage opened, player not teleported");
        Require(game.GetRemainingTime() == beforeTime
            && game.GetStage().GetEndpoints().shortestDistance == beforeDistance,
            "Breaking walls must not change remaining time or original bonus distance");
        Require(!game.TryUseItem(ActiveItemType::WallBreak, target.direction),
            "A single item cannot be reused");
        game.TryMove(target.direction);
        Require(game.GetStage().GetPlayer().GetPosition() == to,
            "Normal movement must traverse the newly opened wall");
        game.TryMove({ -target.direction.x, -target.direction.y });
        Require(game.GetStage().GetPlayer().GetPosition() == player,
            "Opening must remain available for the reverse direction");
        RequirePerimeterIntact(game.GetStage().GetMaze());
    }

    void TestGameOverAndFlowGate()
    {
        GameSession over(FindSessionSeed());
        over.Update(GameTuning::Time::InitialSeconds);
        Require(!over.TryUseItem(ActiveItemType::WallBreak, { 1, 0 }),
            "Game Over cannot use items");

        GameFlow flow;
        Require(!flow.TryUseItem(ActiveItemType::WallBreak, { 1, 0 }),
            "Main menu cannot use Wall Break");
        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(!flow.TryUseItem(ActiveItemType::WallBreak, { 1, 0 }),
            "Countdown must block Wall Break");
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds));
        flow.Update(GameTuning::Transition::GoSeconds);
        flow.Pause();
        Require(!flow.TryUseItem(ActiveItemType::WallBreak, { 1, 0 }),
            "Pause must block Wall Break");
    }
}

int main()
{
    try
    {
        TestSpawnAndPlacement();
        TestPermanentWallChangeAndFailedUses();
        TestCollectCarryBreakAndRecall();
        TestGameOverAndFlowGate();
        std::cout << "Wall Break tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Wall Break test failed: " << error.what() << '\n';
        return 1;
    }
}
