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

    void Require(bool _condition, const char* _message)
    {
        if (!_condition) throw std::runtime_error(_message);
    }

    bool HasItem(const Stage& _stage, RunItemType _item)
    {
        switch (_item)
        {
        case RunItemType::TimeOrb: return _stage.GetTimeOrbPosition().has_value();
        case RunItemType::Recall: return _stage.GetRecallPosition().has_value();
        case RunItemType::WallBreak: return _stage.GetWallBreakPosition().has_value();
        case RunItemType::Teleport: return _stage.GetTeleportPosition().has_value();
        default: return false;
        }
    }

    CellPosition GetItemPosition(const Stage& _stage, RunItemType _item)
    {
        switch (_item)
        {
        case RunItemType::TimeOrb: return *_stage.GetTimeOrbPosition();
        case RunItemType::Recall: return *_stage.GetRecallPosition();
        case RunItemType::WallBreak: return *_stage.GetWallBreakPosition();
        case RunItemType::Teleport: return *_stage.GetTeleportPosition();
        default: throw std::runtime_error("Unsupported test item");
        }
    }

    std::uint32_t FindSeedForItem(RunItemType _item, bool _present = true)
    {
        for (std::uint32_t seed = 0; seed < 20000; ++seed)
        {
            std::mt19937 rng(seed);
            const Stage stage(1, rng());
            if (HasItem(stage, _item) == _present) return seed;
        }
        throw std::runtime_error("Could not find suitable item spawn seed");
    }

    std::vector<CellPosition> FindPath(const Stage& _stage,
        CellPosition _from, CellPosition _to)
    {
        const Maze& maze = _stage.GetMaze();
        const int width = maze.GetCellWidth();
        const int height = maze.GetCellHeight();
        const auto index = [width](CellPosition p) { return p.y * width + p.x; };
        std::vector<int> parent(static_cast<std::size_t>(width * height), -1);
        std::queue<CellPosition> queue;
        parent[static_cast<std::size_t>(index(_from))] = index(_from);
        queue.push(_from);
        while (!queue.empty())
        {
            const CellPosition current = queue.front();
            queue.pop();
            if (current == _to) break;
            for (const CellPosition dir : Directions)
            {
                const CellPosition next{ current.x + dir.x, current.y + dir.y };
                if ((next == _stage.GetEndpoints().goal && next != _to)
                    || !maze.CanMove(current, next)
                    || parent[static_cast<std::size_t>(index(next))] != -1)
                {
                    continue;
                }
                parent[static_cast<std::size_t>(index(next))] = index(current);
                queue.push(next);
            }
        }
        std::vector<CellPosition> steps;
        int pos = index(_to);
        Require(parent[static_cast<std::size_t>(pos)] >= 0,
            "Test path must be reachable without clearing stage early");
        while (pos != index(_from))
        {
            const int previous = parent[static_cast<std::size_t>(pos)];
            steps.push_back({ pos % width - previous % width,
                pos / width - previous / width });
            pos = previous;
        }
        std::reverse(steps.begin(), steps.end());
        return steps;
    }

    void Walk(GameSession& _game, const std::vector<CellPosition>& _steps)
    {
        for (const CellPosition dir : _steps) _game.TryMove(dir);
    }

    void TestSpawnsOnStageCreation()
    {
        for (std::uint32_t seed = 0; seed < 100; ++seed)
        {
            GameSession game(seed);
            Require(game.GetStatistics().GetPlayingSeconds() == 0.0
                && game.GetStatistics().GetSuccessfulMoves() == 0,
                "Run must begin with zero play time and moves");
            for (const RunItemType item : { RunItemType::TimeOrb,
                    RunItemType::Recall, RunItemType::WallBreak,
                    RunItemType::Teleport })
            {
                const RunItemCounters counters = game.GetStatistics().GetItem(item);
                Require(counters.spawned ==
                    static_cast<std::uint64_t>(HasItem(game.GetStage(), item))
                    && counters.pickedUp == 0 && counters.used == 0,
                    "Stage-1 spawned counter must match actual visible item");
            }
        }
    }

    void TestElapsedTimeAndSuccessfulSteps()
    {
        GameSession game(FindSeedForItem(RunItemType::TimeOrb, false));
        const Stage& stage = game.GetStage();
        const CellPosition start = stage.GetPlayer().GetPosition();
        Require(game.GetStatistics().GetPlayingSeconds() == 0.0,
            "Countdown time is excluded from a newly created GameSession");
        game.Update(2.5f);
        Require(game.GetStatistics().GetPlayingSeconds() == 2.5,
            "Active gameplay seconds must accumulate");
        game.TryMove({ 0, 0 });
        game.TryMove({ 99, 0 });
        Require(game.GetStatistics().GetSuccessfulMoves() == 0,
            "Invalid movement must not be counted");

        bool moved = false;
        for (const CellPosition dir : Directions)
        {
            if (!stage.GetMaze().CanMove(start, { start.x + dir.x, start.y + dir.y }))
                continue;
            game.TryMove(dir);
            game.TryMove({ -dir.x, -dir.y });
            moved = true;
            break;
        }
        Require(moved && game.GetStage().GetPlayer().GetPosition() == start
            && game.GetStatistics().GetSuccessfulMoves() == 2,
            "Both successful forward and reverse steps must count");
        game.Update(1.5f);
        Require(game.GetStatistics().GetPlayingSeconds() == 4.0,
            "Multiple play-time updates accumulate without rounding");
        game.Update(100.0f);
        Require(game.GetStatus() == GameStatus::GameOver
            && std::fabs(game.GetStatistics().GetPlayingSeconds()
                - GameTuning::Time::InitialSeconds) < 0.001,
            "Oversized final frame counts only remaining active time");
        game.Update(3.0f);
        game.TryMove({ 1, 0 });
        Require(game.GetStatistics().GetSuccessfulMoves() == 2
            && std::fabs(game.GetStatistics().GetPlayingSeconds()
                - GameTuning::Time::InitialSeconds) < 0.001,
            "Stats cannot change after Game Over");
    }

    void TestOrbSpawnAndPickup()
    {
        GameSession game(FindSeedForItem(RunItemType::TimeOrb));
        const CellPosition pickup = GetItemPosition(game.GetStage(), RunItemType::TimeOrb);
        Walk(game, FindPath(game.GetStage(),
            game.GetStage().GetPlayer().GetPosition(), pickup));
        const RunItemCounters counters = game.GetStatistics().GetItem(RunItemType::TimeOrb);
        Require(counters.spawned == 1 && counters.pickedUp == 1 && counters.used == 0,
            "Time Orb pickup counts once; it has no manual-use counter");
        Require(!game.GetStage().GetTimeOrbPosition(),
            "Orb should be removed once collected");
        game.TryMove({ 0, 0 });
        Require(game.GetStatistics().GetItem(RunItemType::TimeOrb).pickedUp == 1,
            "Failed movement must not duplicate Orb pickup count");
    }

    void TestRecallPickupAndUse()
    {
        GameSession game(FindSeedForItem(RunItemType::Recall));
        const CellPosition pickup = GetItemPosition(game.GetStage(), RunItemType::Recall);
        Walk(game, FindPath(game.GetStage(),
            game.GetStage().GetPlayer().GetPosition(), pickup));
        Require(game.GetStatistics().GetItem(RunItemType::Recall).pickedUp == 1,
            "Actual Recall pickup increments exactly once");
        Require(game.TryUseItem(ActiveItemType::Recall),
            "Recall should rewind from an item Cell");
        const RunItemCounters counters = game.GetStatistics().GetItem(RunItemType::Recall);
        Require(counters.spawned == 1 && counters.pickedUp == 1 && counters.used == 1,
            "Successful Recall use must be recorded");
        Require(!game.TryUseItem(ActiveItemType::Recall)
            && game.GetStatistics().GetItem(RunItemType::Recall).used == 1,
            "Repeated failed Recall cannot inflate usage");
    }

    void TestWallBreakPickupAndUse()
    {
        GameSession game(FindSeedForItem(RunItemType::WallBreak));
        const CellPosition pickup = GetItemPosition(game.GetStage(), RunItemType::WallBreak);
        Walk(game, FindPath(game.GetStage(),
            game.GetStage().GetPlayer().GetPosition(), pickup));
        Require(game.GetStatistics().GetItem(RunItemType::WallBreak).pickedUp == 1,
            "Wall Break pickup increments exactly once");
        Require(!game.TryUseItem(ActiveItemType::WallBreak, { 0, 0 })
            && game.GetStatistics().GetItem(RunItemType::WallBreak).used == 0,
            "Invalid Wall Break must not be counted");

        CellPosition wallFrom{ -1, -1 }, wallDir{ 0, 0 };
        const Stage& stage = game.GetStage();
        const Maze& maze = stage.GetMaze();
        for (int y = 0; y < maze.GetCellHeight() && wallFrom.x == -1; ++y)
        {
            for (int x = 0; x < maze.GetCellWidth() && wallFrom.x == -1; ++x)
            {
                const CellPosition from{ x, y };
                if (from == stage.GetEndpoints().goal) continue;
                for (const CellPosition dir : Directions)
                {
                    const CellPosition to{ x + dir.x, y + dir.y };
                    if (maze.IsCellInside(to) && !maze.CanMove(from, to))
                    {
                        wallFrom = from;
                        wallDir = dir;
                        break;
                    }
                }
            }
        }
        Require(wallFrom.x >= 0, "Expected a destroyable internal wall");
        Walk(game, FindPath(game.GetStage(), pickup, wallFrom));
        Require(game.TryUseItem(ActiveItemType::WallBreak, wallDir)
            && game.GetStatistics().GetItem(RunItemType::WallBreak).used == 1,
            "A successful break counts one item use");
        Require(!game.TryUseItem(ActiveItemType::WallBreak, wallDir)
            && game.GetStatistics().GetItem(RunItemType::WallBreak).used == 1,
            "Cannot count re-breaking an open wall");
    }

    void TestTeleportPickupAndUse()
    {
        GameSession game(FindSeedForItem(RunItemType::Teleport));
        const CellPosition pickup = GetItemPosition(game.GetStage(), RunItemType::Teleport);
        Walk(game, FindPath(game.GetStage(),
            game.GetStage().GetPlayer().GetPosition(), pickup));
        const Stage& stage = game.GetStage();
        Require(game.GetStatistics().GetItem(RunItemType::Teleport).pickedUp == 1,
            "Actual Teleport pickup increments exactly once");
        Require(!game.TryTeleport(stage.GetEndpoints().goal)
            && game.GetStatistics().GetItem(RunItemType::Teleport).used == 0,
            "Rejected Teleport to Goal cannot count as use");
        const CellPosition here = stage.GetPlayer().GetPosition();
        Require(!game.TryTeleport(here)
            && game.GetStatistics().GetItem(RunItemType::Teleport).used == 0,
            "Self-position cancellation does not count as usage");
        bool teleported = false;
        for (const CellPosition dir : Directions)
        {
            const CellPosition target{ here.x + dir.x, here.y + dir.y };
            if (stage.CanTeleportTo(target))
            {
                teleported = game.TryTeleport(target);
                break;
            }
        }
        Require(teleported
            && game.GetStatistics().GetItem(RunItemType::Teleport).used == 1,
            "Successful Teleport counts one use");
        Require(!game.TryTeleport(here)
            && game.GetStatistics().GetItem(RunItemType::Teleport).used == 1,
            "Without inventory Teleport use counter remains fixed");
    }

    void TestStageChangeCountsNewSpawns()
    {
        GameSession game(1u);
        const Stage& initial = game.GetStage();
        const CellPosition start = initial.GetEndpoints().start;
        const CellPosition goal = initial.GetEndpoints().goal;
        const auto path = FindPath(initial, start, goal);
        Walk(game, path);
        Require(game.GetCompletedStages() == 1 && game.GetStage().GetNumber() == 2,
            "Walking to Goal should generate next stage");
        Require(game.GetStatistics().GetSuccessfulMoves() == path.size(),
            "Goal step included in successful movement count");
        for (const RunItemType item : { RunItemType::TimeOrb, RunItemType::Recall,
                RunItemType::WallBreak, RunItemType::Teleport })
        {
            std::mt19937 rng(1u);
            const Stage original(1, rng());
            const auto expectedSpawns =
                static_cast<std::uint64_t>(HasItem(original, item))
                + static_cast<std::uint64_t>(HasItem(game.GetStage(), item));
            Require(game.GetStatistics().GetItem(item).spawned == expectedSpawns,
                "Stage change records the new item's appearance exactly once");
        }
    }

    void TestFlowPauseCountdownAndNewSession()
    {
        GameFlow flow;
        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetSession()
            && flow.GetSession()->GetStatistics().GetPlayingSeconds() == 0.0,
            "Fade and countdown do not add play duration");
        flow.Update(1.0f);
        Require(flow.GetSession()->GetStatistics().GetPlayingSeconds() == 0.0,
            "Countdown is excluded from active time");
        flow.Update(static_cast<float>(GameTuning::Transition::CountdownSeconds));
        flow.Update(GameTuning::Transition::GoSeconds);
        Require(flow.GetState() == ScreenState::Playing
            && flow.GetSession()->GetStatistics().GetPlayingSeconds() == 0.0,
            "GO is excluded from active time");
        flow.Update(2.0f);
        flow.Pause();
        flow.Update(40.0f);
        Require(flow.GetSession()->GetStatistics().GetPlayingSeconds() == 2.0,
            "Pause does not increase the recorded play duration");
        flow.Resume();
        flow.Update(1.0f);
        Require(flow.GetSession()->GetStatistics().GetPlayingSeconds() == 3.0,
            "Play resumes counting after Pause");
        flow.Pause();
        flow.ReturnToMainMenu();
        flow.Update(GameTuning::Transition::FadeSeconds);
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetState() == ScreenState::MainMenu
            && !flow.GetSession(), "Main menu clears the finished run");
        flow.StartGame();
        flow.Update(GameTuning::Transition::FadeSeconds);
        Require(flow.GetSession()
            && flow.GetSession()->GetStatistics().GetPlayingSeconds() == 0.0
            && flow.GetSession()->GetStatistics().GetSuccessfulMoves() == 0,
            "Starting another run resets all nonpersistent counters");
    }
}

int main()
{
    try
    {
        TestSpawnsOnStageCreation();
        TestElapsedTimeAndSuccessfulSteps();
        TestOrbSpawnAndPickup();
        TestRecallPickupAndUse();
        TestWallBreakPickupAndUse();
        TestTeleportPickupAndUse();
        TestStageChangeCountsNewSpawns();
        TestFlowPauseCountdownAndNewSession();
        std::cout << "Run statistics tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Run statistics tests failed: " << error.what() << '\n';
        return 1;
    }
}
