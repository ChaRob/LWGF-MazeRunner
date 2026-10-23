#include "../src/game/maze/MazeGenerator.h"
#include "../src/game/maze/MazeEndpointGenerator.h"
#include "../src/game/player/HeldMovementInput.h"
#include "../src/game/GameSession.h"
#include "../src/game/GameTuning.h"
#include "../src/game/StageClearNotice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr std::array<CellPosition, 4> Directions =
    {
        CellPosition{ 0, 1 },
        CellPosition{ 0, -1 },
        CellPosition{ -1, 0 },
        CellPosition{ 1, 0 }
    };

    void Require(bool _condition, const char* _message)
    {
        if (!_condition)
        {
            throw std::runtime_error(_message);
        }
    }

    void TestDeadEndGoals()
    {
        for (const int size : { 5, 7, 25 })
        {
            for (std::uint32_t seed = 0; seed < 100; ++seed)
            {
                const Maze maze = MazeGenerator::Generate(size, size, seed);
                const MazeEndpoints endpoints =
                    MazeEndpointGenerator::Generate(maze, seed ^ 0x9E3779B9u,
                        GameTuning::Maze::GoalMinimumDistanceRatio);

                Require(endpoints.start != endpoints.goal, "Start and Goal must differ");

                int openNeighbors = 0;
                for (const CellPosition& direction : Directions)
                {
                    const CellPosition neighbor =
                    {
                        endpoints.goal.x + direction.x,
                        endpoints.goal.y + direction.y
                    };

                    if (maze.IsCellInside(neighbor) && maze.CanMove(endpoints.goal, neighbor))
                    {
                        ++openNeighbors;
                    }
                }
                Require(openNeighbors == 1, "Goal must be at a dead end");

                const auto index = [size](const CellPosition& _position)
                {
                    return static_cast<std::size_t>(_position.y * size + _position.x);
                };

                std::vector<int> distances(static_cast<std::size_t>(size * size), -1);
                std::queue<CellPosition> queue;
                distances[index(endpoints.start)] = 0;
                queue.push(endpoints.start);
                int maxDistance = 0;

                while (!queue.empty())
                {
                    const CellPosition current = queue.front();
                    queue.pop();
                    const int distance = distances[index(current)];

                    for (const CellPosition& direction : Directions)
                    {
                        const CellPosition next =
                        {
                            current.x + direction.x,
                            current.y + direction.y
                        };
                        if (!maze.IsCellInside(next) || !maze.CanMove(current, next))
                        {
                            continue;
                        }

                        if (distances[index(next)] != -1)
                        {
                            continue;
                        }

                        distances[index(next)] = distance + 1;
                        maxDistance = std::max(maxDistance, distance + 1);
                        queue.push(next);
                    }
                }

                Require(distances[index(endpoints.goal)] == endpoints.shortestDistance,
                    "Returned Goal distance must match shortest path");
                Require(endpoints.shortestDistance >=
                    static_cast<int>(std::ceil(maxDistance * GameTuning::Maze::GoalMinimumDistanceRatio)),
                    "Goal must preserve the minimum distance policy");
            }
        }
    }

    void TestGameSessionTimeBonus()
    {
        // Spend some initial time so the reward is observable before the cap.
        const float ElapsedSeconds = GameTuning::Time::InitialSeconds * 0.25f;

        GameSession game(12345u);
        const Stage& stage = game.GetStage();
        const Maze& maze = stage.GetMaze();
        const MazeEndpoints& endpoints = stage.GetEndpoints();
        const int width = maze.GetCellWidth();

        const auto index = [width](const CellPosition& _position)
        {
            return _position.y * width + _position.x;
        };

        std::vector<int> parents(
            static_cast<std::size_t>(width * maze.GetCellHeight()), -1);
        std::queue<CellPosition> pending;

        parents[static_cast<std::size_t>(index(endpoints.start))] =
            index(endpoints.start);
        pending.push(endpoints.start);

        while (!pending.empty())
        {
            const CellPosition current = pending.front();
            pending.pop();

            for (const CellPosition& direction : Directions)
            {
                const CellPosition next =
                {
                    current.x + direction.x,
                    current.y + direction.y
                };
                if (!maze.IsCellInside(next) || !maze.CanMove(current, next))
                {
                    continue;
                }

                const std::size_t nextIndex =
                    static_cast<std::size_t>(index(next));
                if (parents[nextIndex] != -1)
                {
                    continue;
                }

                parents[nextIndex] = index(current);
                pending.push(next);
            }
        }

        const int goalIndex = index(endpoints.goal);
        const int startIndex = index(endpoints.start);
        Require(parents[static_cast<std::size_t>(goalIndex)] != -1,
            "Goal must be reachable from Start");

        std::vector<CellPosition> path;
        int currentIndex = goalIndex;
        while (currentIndex != startIndex)
        {
            const int previousIndex = parents[static_cast<std::size_t>(currentIndex)];
            Require(previousIndex != -1, "Path reconstruction failed");
            path.push_back(
            {
                currentIndex % width - previousIndex % width,
                currentIndex / width - previousIndex / width
            });
            currentIndex = previousIndex;
        }
        std::reverse(path.begin(), path.end());
        Require(static_cast<int>(path.size()) == endpoints.shortestDistance,
            "Path length must match the stage shortest distance");

        game.Update(ElapsedSeconds);
        for (const CellPosition& direction : path)
        {
            game.TryMove(direction);
        }

        Require(game.GetCompletedStages() == 1, "Path should clear Stage 1");
        Require(game.GetStage().GetNumber() == 2, "Goal should start Stage 2");

        const float expectedRemainingTime = std::min(GameTuning::Time::MaximumSeconds,
            game.GetInitialTime() - ElapsedSeconds
            + static_cast<float>(path.size()) * GameTuning::Time::ClearBonusSecondsPerCell);
        Require(std::fabs(game.GetRemainingTime() - expectedRemainingTime) < 0.001f,
            "Clear bonus must use the configured seconds per cell");

        const float speedScoreSeconds = std::max(0.0f,
            static_cast<float>(path.size()) * GameTuning::Score::SpeedBaselineSecondsPerCell
            - ElapsedSeconds);
        const std::uint64_t expectedScore =
            GameTuning::Score::BasePointsPerStage
            + static_cast<std::uint64_t>(path.size()) * GameTuning::Score::DistancePointsPerCell
            + static_cast<std::uint64_t>(
                std::floor(speedScoreSeconds * GameTuning::Score::SpeedPointsPerSecondSaved));
        Require(game.GetScore() == expectedScore,
            "Time bonus reduction must not alter existing speed-score formula");
    }

    void TestStageClearNotice()
    {
        StageClearNotice notice;
        Require(!notice.IsVisible(), "Clear notice must start hidden");
        Require(notice.GetOpacity() == 0.0f, "Hidden notice must be transparent");

        notice.Show({ 6, 350, 2.5f });
        Require(notice.IsVisible(), "Clear notice must appear immediately");
        Require(notice.GetReward().completedStage == 6
            && notice.GetReward().earnedPoints == 350
            && std::fabs(notice.GetReward().grantedTimeSeconds - 2.5f) < 0.001f,
            "Clear notice must retain the completed stage and actual rewards");
        Require(notice.GetOpacity() == 1.0f, "New notice must be fully visible");

        notice.Update(0.7f);
        Require(notice.GetOpacity() == 1.0f, "Notice must remain readable before fade");
        notice.Update(0.2f);
        Require(notice.IsVisible() && notice.GetOpacity() > 0.0f
            && notice.GetOpacity() < 1.0f,
            "Notice must fade out near the end of its one-second lifetime");

        // A second clear replaces the previous reward and resets the one-second timer.
        notice.Show({ 7, 420, 0.0f });
        Require(notice.GetReward().completedStage == 7
            && notice.GetReward().earnedPoints == 420
            && notice.GetReward().grantedTimeSeconds == 0.0f,
            "Consecutive clears must replace stale notice data");
        Require(notice.GetOpacity() == 1.0f, "New clear must reset notice opacity");
        notice.Update(GameTuning::Transition::ClearNoticeSeconds * 0.99f);
        Require(notice.IsVisible(), "Notice must last almost the full second");
        notice.Update(GameTuning::Transition::ClearNoticeSeconds * 0.02f);
        Require(!notice.IsVisible(), "Notice must disappear after one second");

        notice.Show({ 8, 500, 1.0f });
        notice.Clear();
        Require(!notice.IsVisible(), "Returning to the menu must dismiss the notice");

        // The notice exists independently of the session and cannot pause the clock.
        GameSession runningGame(6789u);
        const float timeBefore = runningGame.GetRemainingTime();
        notice.Show({ 1, 100, 1.0f });
        runningGame.Update(0.5f);
        notice.Update(0.5f);
        Require(runningGame.GetStatus() == GameStatus::Playing
            && std::fabs(runningGame.GetRemainingTime() - (timeBefore - 0.5f)) < 0.001f,
            "Showing a clear notice must not stop the gameplay timer");
    }

    void TestHeldMovement()
    {
        HeldMovementInput input;
        const float interval = GameTuning::Movement::RepeatIntervalSeconds;
        const float shortDelay = interval * 0.4f;
        const float finishDelay = interval * 0.7f;
        const float fullDelay = interval * 1.1f;
        const MovementButtons none = {};
        const MovementButtons up = { true, false, false, false };
        const MovementButtons upAndRight = { true, false, false, true };
        const MovementButtons right = { false, false, false, true };

        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Initial neutral input must not move");
        Require(input.Update(0.0f, up) == CellPosition{ 0, 1 },
            "First keypress must move immediately");
        Require(input.Update(shortDelay, up) == CellPosition{ 0, 0 },
            "Hold must not repeat before configured interval");
        Require(input.Update(finishDelay, up) == CellPosition{ 0, 1 },
            "Hold must repeat after configured interval");

        Require(input.Update(0.0f, upAndRight) == CellPosition{ 0, 0 },
            "Direction change must respect shared cooldown");
        Require(input.Update(fullDelay, upAndRight) == CellPosition{ 1, 0 },
            "Newest direction should move when shared cooldown expires");

        Require(input.Update(0.0f, up) == CellPosition{ 0, 0 },
            "Returning to a previous direction must respect cooldown");
        Require(input.Update(fullDelay, up) == CellPosition{ 0, 1 },
            "Previous held direction should resume after cooldown");

        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Releasing all keys must stop immediately");
        Require(input.Update(0.0f, right) == CellPosition{ 0, 0 },
            "Re-pressing a key must not bypass the shared cooldown");
        Require(input.Update(fullDelay, right) == CellPosition{ 1, 0 },
            "Re-pressed key must move once interval elapses");

        Require(input.Update(interval * 3.0f, right) == CellPosition{ 1, 0 },
            "Long frame must issue at most one step");
        Require(input.Update(0.0f, right) == CellPosition{ 0, 0 },
            "Long frame must not queue extra movement");
        Require(input.Update(fullDelay, none) == CellPosition{ 0, 0 },
            "Idle time must not cause movement");
        Require(input.Update(0.0f, up) == CellPosition{ 0, 1 },
            "First press after enough idle time must move immediately");

        input.RequireRelease(); // Stage change or pause.
        Require(input.Update(1.0f, up) == CellPosition{ 0, 0 },
            "Held key must not move after stage switch");
        Require(input.Update(1.0f, up) == CellPosition{ 0, 0 },
            "Held key must remain blocked until release");
        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Key release must only unlock input");
        Require(input.Update(0.0f, right) == CellPosition{ 1, 0 },
            "Fresh press after release must move immediately");
    }
}

int main()
{
    try
    {
        TestDeadEndGoals();
        TestHeldMovement();
        TestStageClearNotice();
        TestGameSessionTimeBonus();
        std::cout << "Gameplay improvement tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Gameplay improvement test failed: " << error.what() << '\n';
        return 1;
    }
}
