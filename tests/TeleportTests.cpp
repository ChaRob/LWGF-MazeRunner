#include "../src/game/GameSession.h"
#include "../src/game/GameFlow.h"
#include "../src/game/GameTuning.h"

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
        int current = index(_to);
        Require(parents[static_cast<std::size_t>(current)] != -1,
            "Target must be reachable");
        std::vector<CellPosition> result;
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

    void Walk(GameSession& _game, const std::vector<CellPosition>& _path)
    {
        for (const CellPosition direction : _path)
        {
            _game.TryMove(direction);
        }
    }

    std::uint32_t SeedWithTeleport()
    {
        for (std::uint32_t seed = 0; seed < 10000; ++seed)
        {
            std::mt19937 rng(seed);
            const Stage stage(1, rng());
            if (stage.GetTeleportPosition())
            {
                return seed;
            }
        }
        throw std::runtime_error("Could not find first-stage Teleport spawn");
    }

    CellPosition FindValidDestination(const Stage& _stage)
    {
        const CellPosition from = _stage.GetPlayer().GetPosition();
        const Maze& maze = _stage.GetMaze();
        for (int y = 0; y < maze.GetCellHeight(); ++y)
        {
            for (int x = 0; x < maze.GetCellWidth(); ++x)
            {
                const CellPosition dest{ x, y };
                if (_stage.CanTeleportTo(dest)
                    && std::abs(from.x - x) + std::abs(from.y - y) >= 2
                    && (! _stage.GetTimeOrbPosition()
                        || dest != *_stage.GetTimeOrbPosition()))
                {
                    return dest;
                }
            }
        }
        throw std::runtime_error("Expected valid multi-cell teleport destination");
    }

    void TestSpawnRatesAndSeparation()
    {
        using namespace GameTuning::Teleport;
        Require(GetSpawnChance(1) == 0.02 && GetSpawnChance(30) == 0.02,
            "Early Teleport rate must be 2%");
        Require(GetSpawnChance(31) == 0.03 && GetSpawnChance(40) == 0.03,
            "Middle Teleport rate must be 3%");
        Require(GetSpawnChance(41) == 0.05 && GetSpawnChance(50) == 0.05,
            "Late Teleport rate must be 5%");
        Require(GetSpawnChance(51) == 0.075 && GetSpawnChance(999) == 0.075,
            "Endgame Teleport rate must be 7.5%");
        constexpr int SampleCount = 1600;
        for (const std::uint64_t stageNumber : { 1ull, 31ull, 41ull, 51ull })
        {
            int spawned = 0;
            for (int seed = 0; seed < SampleCount; ++seed)
            {
                const Stage stage(stageNumber, static_cast<std::uint32_t>(seed));
                const Stage repeat(stageNumber, static_cast<std::uint32_t>(seed));
                Require(stage.GetTeleportPosition() == repeat.GetTeleportPosition(),
                    "Teleport placement must be reproducible");
                if (!stage.GetTeleportPosition())
                {
                    continue;
                }
                ++spawned;
                const CellPosition item = *stage.GetTeleportPosition();
                Require(stage.GetMaze().IsCellInside(item)
                    && item != stage.GetEndpoints().start
                    && item != stage.GetEndpoints().goal,
                    "Teleport pickup must be a valid non-endpoint Cell");
                Require(!stage.GetTimeOrbPosition()
                    || item != *stage.GetTimeOrbPosition(),
                    "Teleport must not overlap Time Orb");
                Require(!stage.GetRecallPosition()
                    || item != *stage.GetRecallPosition(),
                    "Teleport must not overlap Recall");
                Require(!stage.GetWallBreakPosition()
                    || item != *stage.GetWallBreakPosition(),
                    "Teleport must not overlap Wall Break");
            }
            Require(std::fabs(static_cast<double>(spawned) / SampleCount
                - GetSpawnChance(stageNumber)) < 0.035,
                "Observed Teleport spawn rate differs substantially");
        }
    }

    void TestPickupInvalidTargetsAndTeleport()
    {
        GameSession game(SeedWithTeleport());
        const Stage& stage = game.GetStage();
        const CellPosition start = stage.GetEndpoints().start;
        const CellPosition pickup = *stage.GetTeleportPosition();
        Require(!game.TryTeleport(pickup),
            "Teleport cannot be used without inventory");
        Walk(game, FindPath(stage.GetMaze(), start, pickup));
        Require(game.WasTeleportCollectedOnLastMove()
            && !game.GetStage().GetTeleportPosition()
            && game.GetInventory().GetCount(ActiveItemType::Teleport) == 1,
            "Teleport pickup must increment inventory and vanish");

        const Stage& now = game.GetStage();
        const CellPosition current = now.GetPlayer().GetPosition();
        const CellPosition goal = now.GetEndpoints().goal;
        const Maze& maze = now.GetMaze();
        Require(!now.CanTeleportTo(goal)
            && !game.TryTeleport(goal),
            "Teleport must never jump directly to Goal");
        Require(!game.TryTeleport(current)
            && !game.TryTeleport({ -1, current.y })
            && !game.TryTeleport({ maze.GetCellWidth(), current.y }),
            "Teleport must reject the same Cell and outside positions");
        Require(!game.TryUseItem(ActiveItemType::Teleport),
            "Teleport without destination must not consume inventory");

        const CellPosition dest = FindValidDestination(now);
        const float previousTime = game.GetRemainingTime();
        const int initialDistance = now.GetEndpoints().shortestDistance;
        const Maze copy = maze;
        Require(game.TryTeleport(dest), "Teleport must reach a valid distant Cell");
        Require(game.GetStage().GetPlayer().GetPosition() == dest
            && game.GetInventory().GetCount(ActiveItemType::Teleport) == 0,
            "Teleport should consume one item and reposition instantly");
        Require(game.GetRemainingTime() == previousTime
            && game.GetCompletedStages() == 0
            && game.GetStage().GetEndpoints().shortestDistance == initialDistance,
            "Teleport must not change timer, goal rewards or stage number");
        for (int y = 0; y < maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < maze.GetTileWidth(); ++x)
            {
                Require(maze.GetTile(x, y) == copy.GetTile(x, y),
                    "Teleport must leave every maze wall unchanged");
            }
        }
        Require(!game.TryTeleport(current), "A used Teleport cannot be reused");
    }

    void TestCarryOverAndTimeout()
    {
        GameSession game(SeedWithTeleport());
        const Maze& firstMaze = game.GetStage().GetMaze();
        const CellPosition start = game.GetStage().GetEndpoints().start;
        const CellPosition pickup = *game.GetStage().GetTeleportPosition();
        const CellPosition goal = game.GetStage().GetEndpoints().goal;
        Walk(game, FindPath(firstMaze, start, pickup));
        Walk(game, FindPath(firstMaze, pickup, goal));
        Require(game.GetStage().GetNumber() == 2
            && game.GetInventory().GetCount(ActiveItemType::Teleport) == 1,
            "Unused Teleport must carry to next stage");
        const CellPosition dest = FindValidDestination(game.GetStage());
        Require(game.TryTeleport(dest)
            && game.GetStage().GetPlayer().GetPosition() == dest
            && game.GetInventory().GetCount(ActiveItemType::Teleport) == 0,
            "Carried Teleport must work on another stage");

        GameSession expired(SeedWithTeleport());
        const Stage& eStage = expired.GetStage();
        Walk(expired, FindPath(eStage.GetMaze(),
            eStage.GetEndpoints().start, *eStage.GetTeleportPosition()));
        Require(expired.GetInventory().GetCount(ActiveItemType::Teleport) == 1,
            "Precondition: expired test has one Teleport");
        expired.Update(GameTuning::Time::InitialSeconds);
        Require(expired.GetStatus() == GameStatus::GameOver
            && !expired.TryTeleport(FindValidDestination(expired.GetStage())),
            "Teleport must not revive an expired run");
        GameSession fresh(SeedWithTeleport());
        Require(fresh.GetInventory().GetCount(ActiveItemType::Teleport) == 0,
            "New game must not inherit previous run inventory");
    }

    void TestFlowGates()
    {
        GameFlow flow;
        Require(!flow.TryTeleport({ 1, 1 }), "Main menu must reject Teleport");
        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(!flow.TryTeleport({ 1, 1 }), "Countdown must reject Teleport");
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds));
        flow.Update(GameTuning::Transition::GoSeconds);
        flow.Pause();
        Require(!flow.TryTeleport({ 1, 1 }), "Pause must reject Teleport");
    }
}

int main()
{
    try
    {
        TestSpawnRatesAndSeparation();
        TestPickupInvalidTargetsAndTeleport();
        TestCarryOverAndTimeout();
        TestFlowGates();
        std::cout << "Teleport tests passed.\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Teleport tests failed: " << e.what() << '\n';
        return 1;
    }
}
