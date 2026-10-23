#include "../src/game/GameFlow.h"
#include "../src/game/GameSession.h"
#include "../src/game/GameTuning.h"
#include "../src/game/items/ActiveItemInventory.h"

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

    void Require(bool _condition, const char* _message)
    {
        if (!_condition)
        {
            throw std::runtime_error(_message);
        }
    }

    std::vector<CellPosition> FindPath(const Maze& _maze,
        CellPosition _start, CellPosition _goal)
    {
        const int width = _maze.GetCellWidth();
        const int height = _maze.GetCellHeight();
        const auto index = [width](CellPosition _pos) {
            return _pos.y * width + _pos.x;
        };

        std::vector<int> parent(static_cast<std::size_t>(width * height), -1);
        std::queue<CellPosition> pending;
        pending.push(_start);
        parent[static_cast<std::size_t>(index(_start))] = index(_start);
        while (!pending.empty())
        {
            const CellPosition cell = pending.front();
            pending.pop();
            if (cell == _goal)
            {
                break;
            }
            for (const CellPosition direction : Directions)
            {
                const CellPosition next{ cell.x + direction.x, cell.y + direction.y };
                if (!_maze.CanMove(cell, next))
                {
                    continue;
                }
                const int child = index(next);
                if (parent[static_cast<std::size_t>(child)] != -1)
                {
                    continue;
                }
                parent[static_cast<std::size_t>(child)] = index(cell);
                pending.push(next);
            }
        }
        int current = index(_goal);
        const int start = index(_start);
        Require(parent[static_cast<std::size_t>(current)] != -1,
            "All cells in a perfect maze must be reachable");
        std::vector<CellPosition> result;
        while (current != start)
        {
            const int previous = parent[static_cast<std::size_t>(current)];
            result.push_back({ current % width - previous % width,
                current / width - previous / width });
            current = previous;
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    void TestInventoryCaps()
    {
        ActiveItemInventory inventory;
        const std::array<ActiveItemType, 3> types{
            ActiveItemType::Recall, ActiveItemType::WallBreak, ActiveItemType::Teleport
        };
        for (const ActiveItemType type : types)
        {
            const int capacity = ActiveItemInventory::GetCapacity(type);
            Require(capacity > 0 && inventory.GetCount(type) == 0,
                "Inventory must start empty with positive per-type capacities");
            Require(!inventory.TryConsume(type),
                "Empty items cannot be consumed");
            for (int i = 0; i < capacity; ++i)
            {
                Require(inventory.TryAdd(type), "Item may be added below capacity");
            }
            Require(!inventory.TryAdd(type), "Overfilling any item must fail");
            Require(inventory.GetCount(type) == capacity,
                "Cap must be enforced independently for each item type");
            Require(inventory.TryConsume(type) && inventory.GetCount(type) == capacity - 1,
                "Successfully using an item removes exactly one");
        }
        Require(inventory.GetCount(ActiveItemType::Recall) == 1
            && inventory.GetCount(ActiveItemType::WallBreak) == 1
            && inventory.GetCount(ActiveItemType::Teleport) == 0,
            "Each item type must have an independent count");
    }

    void TestSpawnRatesAndPlacement()
    {
        using namespace GameTuning::Recall;
        Require(GetSpawnChance(1) == 0.05 && GetSpawnChance(30) == 0.05,
            "Recall has a five percent early tier");
        Require(GetSpawnChance(31) == 0.075 && GetSpawnChance(40) == 0.075,
            "Recall has a 7.5 percent middle tier");
        Require(GetSpawnChance(41) == 0.10 && GetSpawnChance(300) == 0.10,
            "Recall has a ten percent late tier");

        constexpr int SampleCount = 1200;
        for (const std::uint64_t stageNumber : { 1ull, 31ull, 41ull })
        {
            int spawned = 0;
            int both = 0;
            for (int seed = 0; seed < SampleCount; ++seed)
            {
                const Stage stage(stageNumber, static_cast<std::uint32_t>(seed));
                const Stage repeated(stageNumber, static_cast<std::uint32_t>(seed));
                Require(stage.GetRecallPosition() == repeated.GetRecallPosition()
                    && stage.GetTimeOrbPosition() == repeated.GetTimeOrbPosition(),
                    "Stage item placement must be seed-reproducible");
                if (!stage.GetRecallPosition())
                {
                    continue;
                }
                ++spawned;
                const CellPosition position = *stage.GetRecallPosition();
                Require(stage.GetMaze().IsCellInside(position)
                    && position != stage.GetEndpoints().start
                    && position != stage.GetEndpoints().goal,
                    "Recall must spawn on a valid non-endpoint Cell");
                if (stage.GetTimeOrbPosition())
                {
                    ++both;
                    Require(position != *stage.GetTimeOrbPosition(),
                        "Recall and Time Orb may co-spawn but cannot overlap");
                }
            }
            const double observed = static_cast<double>(spawned) / SampleCount;
            Require(std::fabs(observed - GetSpawnChance(stageNumber)) < 0.04,
                "Recall spawn rate is inconsistent with configured probability");
            // Both types have independent rolls. Do not require co-spawn within
            // every 1200 trials: small probabilities make that probabilistic.
            (void)both;
        }
    }

    std::uint32_t FindSessionSeed(bool _requireOrb)
    {
        for (std::uint32_t seed = 0; seed < 10000u; ++seed)
        {
            std::mt19937 rng(seed);
            const Stage stage(1, rng());
            if (stage.GetRecallPosition().has_value()
                && (stage.GetTimeOrbPosition().has_value() == _requireOrb))
            {
                return seed;
            }
        }
        throw std::runtime_error("Could not find deterministic first-stage Recall");
    }

    void Walk(GameSession& _game, const std::vector<CellPosition>& _path)
    {
        for (const CellPosition direction : _path)
        {
            _game.TryMove(direction);
        }
    }

    void TestPickupRecallRewindAndCarry()
    {
        GameSession game(FindSessionSeed(false));
        const Stage& first = game.GetStage();
        const CellPosition start = first.GetEndpoints().start;
        const CellPosition recallPosition = *first.GetRecallPosition();
        const std::vector<CellPosition> path = FindPath(first.GetMaze(), start, recallPosition);

        Require(!game.TryUseItem(ActiveItemType::Recall),
            "Recall must fail without an item");
        Require(!game.TryUseItem(ActiveItemType::WallBreak)
            && !game.TryUseItem(ActiveItemType::Teleport),
            "Future item types must not be usable until implemented");
        game.Update(21.0f);
        Walk(game, path);

        Require(game.WasRecallCollectedOnLastMove(),
            "Entering Recall Cell must emit pickup event");
        Require(!game.GetStage().GetRecallPosition()
            && game.GetInventory().GetCount(ActiveItemType::Recall) == 1,
            "Recall pickup must disappear from stage and increase inventory");
        Require(std::fabs(game.GetRemainingTime() - 9.0f) < 0.001f,
            "Collecting Recall must not heal time immediately");

        const int mazeWidth = game.GetStage().GetMaze().GetTileWidth();
        const int mazeHeight = game.GetStage().GetMaze().GetTileHeight();
        const auto goal = game.GetStage().GetEndpoints().goal;

        Require(game.TryUseItem(ActiveItemType::Recall),
            "Space action should use acquired Recall");
        Require(game.GetStage().GetPlayer().GetPosition() == start
            && std::fabs(game.GetRemainingTime() - 30.0f) < 0.001f,
            "Recall must rewind to stage start and stage entry clock");
        Require(game.GetStage().GetMaze().GetTileWidth() == mazeWidth
            && game.GetStage().GetMaze().GetTileHeight() == mazeHeight
            && game.GetStage().GetEndpoints().goal == goal,
            "Recall must not regenerate maze or endpoints");
        Require(!game.GetStage().GetRecallPosition()
            && game.GetInventory().GetCount(ActiveItemType::Recall) == 0,
            "Recall must consume one without regenerating item pickups");
        Require(!game.TryUseItem(ActiveItemType::Recall),
            "Used Recall cannot be reused");

        Walk(game, path);
        Require(game.GetInventory().GetCount(ActiveItemType::Recall) == 0,
            "Revisiting collected Recall must not duplicate it");

        // Run through Stage 1 and ensure inventory isn't cleared at transition.
        const auto finish = FindPath(game.GetStage().GetMaze(),
            game.GetStage().GetPlayer().GetPosition(), goal);
        Walk(game, finish);
        Require(game.GetCompletedStages() == 1 && game.GetStage().GetNumber() == 2,
            "Goal must advance stage without rerolling collected items");
        Require(std::fabs(game.GetStageEntryTime() - game.GetRemainingTime()) < 0.001f,
            "Stage entry time must snapshot the post-clear bonus clock");
        Require(game.GetInventory().GetCount(ActiveItemType::Recall) == 0,
            "Spent items stay spent between stages");
    }

    void TestTimeOrbDoesNotDuplicateOnRewind()
    {
        GameSession game(FindSessionSeed(true));
        const Maze& maze = game.GetStage().GetMaze();
        const CellPosition start = game.GetStage().GetEndpoints().start;
        const CellPosition orb = *game.GetStage().GetTimeOrbPosition();
        const CellPosition recall = *game.GetStage().GetRecallPosition();
        Walk(game, FindPath(maze, start, recall));
        Require(game.GetInventory().GetCount(ActiveItemType::Recall) == 1,
            "Recall must be in inventory before rewind");
        const CellPosition pos = game.GetStage().GetPlayer().GetPosition();
        Walk(game, FindPath(maze, pos, orb));
        Require(!game.GetStage().GetTimeOrbPosition(),
            "Time Orb should be collected only once");
        Require(game.TryUseItem(ActiveItemType::Recall),
            "Recall should return to start after both items are collected");
        Require(!game.GetStage().GetTimeOrbPosition()
            && !game.GetStage().GetRecallPosition(),
            "All collected pickups must stay collected after rewind");
        game.Update(1.0f);
        const float before = game.GetRemainingTime();
        Walk(game, FindPath(maze, start, orb));
        Require(std::fabs(game.GetRemainingTime() - before) < 0.001f,
            "Collected Time Orb cannot grant additional time after Recall");
    }

    void TestNoUseAtStartOrAfterTimeout()
    {
        // A Recall item in the same run must not be consumed at the start.
        GameSession game(FindSessionSeed(false));
        const CellPosition start = game.GetStage().GetEndpoints().start;
        const auto path = FindPath(game.GetStage().GetMaze(),
            start, *game.GetStage().GetRecallPosition());
        Walk(game, path);
        Require(game.TryUseItem(ActiveItemType::Recall),
            "First recall use must work after moving away");
        Require(!game.TryUseItem(ActiveItemType::Recall),
            "Cannot use an item from starting Cell or with no inventory");
        GameSession expired(FindSessionSeed(false));
        expired.Update(GameTuning::Time::MaximumSeconds);
        Require(!expired.TryUseItem(ActiveItemType::Recall),
            "Cannot use Recall to revive a GameOver run");
    }

    void TestFlowStateGate()
    {
        GameFlow flow;
        Require(!flow.TryUseItem(ActiveItemType::Recall),
            "Menu cannot use active abilities");
        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(!flow.TryUseItem(ActiveItemType::Recall),
            "Countdown cannot use active abilities");
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds));
        flow.Update(GameTuning::Transition::GoSeconds);
        flow.Pause();
        Require(!flow.TryUseItem(ActiveItemType::Recall),
            "Paused game cannot use active abilities");
    }
}

int main()
{
    try
    {
        TestInventoryCaps();
        TestSpawnRatesAndPlacement();
        TestPickupRecallRewindAndCarry();
        TestTimeOrbDoesNotDuplicateOnRewind();
        TestNoUseAtStartOrAfterTimeout();
        TestFlowStateGate();
        std::cout << "Recall and inventory tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Recall tests failed: " << error.what() << '\n';
        return 1;
    }
}
