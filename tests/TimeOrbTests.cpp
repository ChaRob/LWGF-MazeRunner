#include "../src/game/GameSession.h"
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
        const auto index = [width](CellPosition _pos) -> int
        {
            return _pos.y * width + _pos.x;
        };

        std::vector<int> parents(static_cast<std::size_t>(width * height), -1);
        std::queue<CellPosition> queue;
        parents[static_cast<std::size_t>(index(_start))] = index(_start);
        queue.push(_start);

        while (!queue.empty())
        {
            const CellPosition current = queue.front();
            queue.pop();
            if (current == _goal)
            {
                break;
            }
            for (const CellPosition direction : Directions)
            {
                const CellPosition next{ current.x + direction.x,
                    current.y + direction.y };
                if (!_maze.CanMove(current, next))
                {
                    continue;
                }
                const std::size_t nextIndex = static_cast<std::size_t>(index(next));
                if (parents[nextIndex] != -1)
                {
                    continue;
                }
                parents[nextIndex] = index(current);
                queue.push(next);
            }
        }

        const int startIndex = index(_start);
        int currentIndex = index(_goal);
        Require(parents[static_cast<std::size_t>(currentIndex)] != -1,
            "The Orb must be reachable by a valid maze corridor");

        std::vector<CellPosition> path;
        while (currentIndex != startIndex)
        {
            const int previous = parents[static_cast<std::size_t>(currentIndex)];
            path.push_back({
                (currentIndex % width) - (previous % width),
                (currentIndex / width) - (previous / width)
            });
            currentIndex = previous;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    void TestSpawnTiersAndPlacement()
    {
        using namespace GameTuning::TimeOrb;
        Require(GetSpawnChance(1) == 0.10 && GetSpawnChance(30) == 0.10,
            "Stages 1-30 must roll ten percent");
        Require(GetSpawnChance(31) == 0.15 && GetSpawnChance(40) == 0.15,
            "Stages 31-40 must roll fifteen percent");
        Require(GetSpawnChance(41) == 0.25 && GetSpawnChance(50) == 0.25,
            "Stages 41-50 must roll twenty-five percent");
        Require(GetSpawnChance(51) == 0.35 && GetSpawnChance(100) == 0.35,
            "Stages 51+ must roll thirty-five percent");

        // Deterministic Bernoulli draws should converge toward configured rates.
        // The tolerance is wider than normal sampling noise to avoid flaky tests.
        constexpr int SampleCount = 1000;
        const std::array<std::uint64_t, 4> stageNumbers = { 1, 31, 41, 51 };
        for (const std::uint64_t stageNumber : stageNumbers)
        {
            int spawned = 0;
            for (int seed = 0; seed < SampleCount; ++seed)
            {
                const Stage stage(stageNumber, static_cast<std::uint32_t>(seed));
                const Stage stageAgain(stageNumber, static_cast<std::uint32_t>(seed));
                Require(stage.GetTimeOrbPosition() == stageAgain.GetTimeOrbPosition(),
                    "Identical stage and seed must reproduce Orb placement");
                if (!stage.GetTimeOrbPosition())
                {
                    continue;
                }
                ++spawned;
                const CellPosition position = *stage.GetTimeOrbPosition();
                Require(stage.GetMaze().IsCellInside(position),
                    "Orb must occupy a traversable logical Cell");
                Require(position != stage.GetEndpoints().start
                    && position != stage.GetEndpoints().goal,
                    "Orb must never cover Start or Goal");
            }
            const double observed = static_cast<double>(spawned) / SampleCount;
            Require(std::fabs(observed - GetSpawnChance(stageNumber)) < 0.05,
                "Observed Orb spawn rate differs substantially from tuning");
        }
    }

    std::uint32_t SeedWithFirstStageOrb()
    {
        for (std::uint32_t sessionSeed = 0; sessionSeed < 1000; ++sessionSeed)
        {
            std::mt19937 random(sessionSeed);
            const Stage stage(1, random());
            if (stage.GetTimeOrbPosition())
            {
                return sessionSeed;
            }
        }
        throw std::runtime_error("Unable to find a seeded first-stage Orb");
    }

    void TestPickupAndTimeCap()
    {
        const std::uint32_t seed = SeedWithFirstStageOrb();
        GameSession game(seed);
        const Stage& stage = game.GetStage();
        Require(stage.GetTimeOrbPosition().has_value(),
            "The selected game seed should spawn an Orb");

        const CellPosition start = stage.GetEndpoints().start;
        const CellPosition orb = *stage.GetTimeOrbPosition();
        const std::vector<CellPosition> path = FindPath(stage.GetMaze(), start, orb);
        Require(!path.empty(), "Orb must differ from Start");

        game.Update(25.0f);
        Require(std::fabs(game.GetRemainingTime() - 5.0f) < 0.001f,
            "Time consumption before picking up Orb must work normally");
        for (std::size_t i = 0; i < path.size(); ++i)
        {
            game.TryMove(path[i]);
            if (i + 1 < path.size())
            {
                Require(!game.WasTimeOrbCollectedOnLastMove(),
                    "Orb cannot be picked up until its own Cell is entered");
            }
        }
        Require(game.WasTimeOrbCollectedOnLastMove(),
            "Walking onto the Orb must report a pickup event");
        Require(std::fabs(game.GetLastTimeOrbGrantedSeconds() - 5.0f) < 0.001f,
            "Pickup must grant the full five seconds below the cap");
        Require(std::fabs(game.GetRemainingTime() - 10.0f) < 0.001f,
            "Pickup must increase the remaining time by five seconds");
        Require(!game.GetStage().GetTimeOrbPosition(),
            "Collected Orb must immediately disappear");
        Require(game.GetCompletedStages() == 0,
            "Picking up an Orb must not clear the stage");

        // Revisit the collected tile: it may not award time twice.
        const CellPosition lastDirection = path.back();
        game.TryMove({ -lastDirection.x, -lastDirection.y });
        Require(!game.WasTimeOrbCollectedOnLastMove(),
            "Walking back must clear the one-move pickup notification");
        game.TryMove(lastDirection);
        Require(!game.WasTimeOrbCollectedOnLastMove()
            && game.GetLastTimeOrbGrantedSeconds() == 0.0f
            && std::fabs(game.GetRemainingTime() - 10.0f) < 0.001f,
            "A previously collected Orb must never restore time again");

        GameSession capped(seed);
        const Stage& cappedStage = capped.GetStage();
        const auto cappedPath = FindPath(cappedStage.GetMaze(),
            cappedStage.GetEndpoints().start, *cappedStage.GetTimeOrbPosition());
        for (const CellPosition direction : cappedPath)
        {
            capped.TryMove(direction);
        }
        Require(capped.WasTimeOrbCollectedOnLastMove()
            && capped.GetLastTimeOrbGrantedSeconds() == 0.0f,
            "Orb pickup at 30 seconds must consume the Orb but grant zero time");
        Require(capped.GetRemainingTime() == GameTuning::Time::MaximumSeconds,
            "Orb must not exceed the hard maximum time");

        GameSession expired(seed);
        expired.Update(GameTuning::Time::InitialSeconds);
        for (const CellPosition direction : path)
        {
            expired.TryMove(direction);
        }
        Require(expired.GetStatus() == GameStatus::GameOver
            && !expired.WasTimeOrbCollectedOnLastMove()
            && expired.GetRemainingTime() == 0.0f,
            "Time Orbs must not revive an expired run");
    }
}

int main()
{
    try
    {
        TestSpawnTiersAndPlacement();
        TestPickupAndTimeCap();
        std::cout << "Time Orb tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Time Orb tests failed: " << error.what() << '\n';
        return 1;
    }
}
