#include "../src/game/maze/MazeGenerator.h"
#include "../src/game/maze/MazeEndpointGenerator.h"
#include "../src/game/player/HeldMovementInput.h"

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
        Require(input.Update(0.06f, up) == CellPosition{ 0, 0 },
            "Hold must not repeat before the interval");
        Require(input.Update(0.06f, up) == CellPosition{ 0, 1 },
            "Hold must repeat after 0.12 seconds");
        Require(input.Update(0.5f, up) == CellPosition{ 0, 1 },
            "Long frame must issue at most one step");
        Require(input.Update(0.0f, up) == CellPosition{ 0, 0 },
            "Long frame must not cause a backlog of steps");

        Require(input.Update(0.0f, upAndRight) == CellPosition{ 1, 0 },
            "Newly pressed direction must override held key");
        Require(input.Update(0.0f, up) == CellPosition{ 0, 1 },
            "Releasing newest key must return to previously held direction");

        Require(input.Update(0.0f, none) == CellPosition{ 0, 0 },
            "Releasing all keys must stop immediately");
        Require(input.Update(0.0f, right) == CellPosition{ 1, 0 },
            "Next new press must move immediately");

        input.RequireRelease(); // Stage change or pause.
        Require(input.Update(1.0f, right) == CellPosition{ 0, 0 },
            "Held key must not move after stage switch");
        Require(input.Update(1.0f, right) == CellPosition{ 0, 0 },
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
        std::cout << "Gameplay improvement tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Gameplay improvement test failed: " << error.what() << '\n';
        return 1;
    }
}
