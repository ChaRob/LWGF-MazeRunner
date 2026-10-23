#include "../src/game/maze/MazeGenerator.h"
#include "../src/game/maze/MazeEndpointGenerator.h"
#include "../src/game/player/HeldMovementInput.h"
#include "../src/game/GameSession.h"

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
                    MazeEndpointGenerator::Generate(maze, seed ^ 0x9E3779B9u, 0.9f);

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
                    static_cast<int>(std::ceil(maxDistance * 0.9f)),
                    "Goal must preserve the minimum distance policy");
            }
        }
    }

    void TestGameSessionTimeBonus()
    {
        // Use enough elapsed time to avoid the 30-second cap masking the award.
        constexpr float ElapsedSeconds = 8.0f;
        constexpr float BonusSecondsPerCell = 0.20f;
        constexpr float SpeedScoreBaselineSecondsPerCell = 0.5f;

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

        const float expectedRemainingTime =
            game.GetInitialTime() - ElapsedSeconds
            + static_cast<float>(path.size()) * BonusSecondsPerCell;
        Require(std::fabs(game.GetRemainingTime() - expectedRemainingTime) < 0.001f,
            "Clear bonus should grant only 0.20 seconds per cell");

        const float speedScoreSeconds = std::max(0.0f,
            static_cast<float>(path.size()) * SpeedScoreBaselineSecondsPerCell
            - ElapsedSeconds);
        const std::uint64_t expectedScore =
            100 + static_cast<std::uint64_t>(path.size()) * 10
            + static_cast<std::uint64_t>(std::floor(speedScoreSeconds * 10.0f));
        Require(game.GetScore() == expectedScore,
            "Time bonus reduction must not alter existing speed-score formula");
    }

    void TestHeldMovement()
    {
        HeldMovementInput input;
        const MovementButtons none = {};
        const MovementButtons up = { true, false, false, false };
        const MovementButtons upAndRight = { true, false, false, true };
        const MovementButtons right = { false, false, false, true };

        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Initial neutral input must not move");
        Require(input.Update(0.0f, up) == CellPosition{ 0, 1 },
            "First keypress must move immediately");
        Require(input.Update(0.07f, up) == CellPosition{ 0, 0 },
            "Hold must not repeat before 0.18 seconds");
        Require(input.Update(0.12f, up) == CellPosition{ 0, 1 },
            "Hold must repeat after 0.18 seconds");

        Require(input.Update(0.0f, upAndRight) == CellPosition{ 0, 0 },
            "Direction change must respect shared cooldown");
        Require(input.Update(0.19f, upAndRight) == CellPosition{ 1, 0 },
            "Newest direction should move when shared cooldown expires");

        Require(input.Update(0.0f, up) == CellPosition{ 0, 0 },
            "Returning to a previous direction must respect cooldown");
        Require(input.Update(0.19f, up) == CellPosition{ 0, 1 },
            "Previous held direction should resume after cooldown");

        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Releasing all keys must stop immediately");
        Require(input.Update(0.0f, right) == CellPosition{ 0, 0 },
            "Re-pressing a key must not bypass the shared cooldown");
        Require(input.Update(0.19f, right) == CellPosition{ 1, 0 },
            "Re-pressed key must move once interval elapses");

        Require(input.Update(0.5f, right) == CellPosition{ 1, 0 },
            "Long frame must issue at most one step");
        Require(input.Update(0.0f, right) == CellPosition{ 0, 0 },
            "Long frame must not queue extra movement");
        Require(input.Update(0.19f, none) == CellPosition{ 0, 0 },
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
