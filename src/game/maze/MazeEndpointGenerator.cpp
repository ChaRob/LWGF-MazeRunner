#include "MazeEndpointGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <queue>
#include <random>
#include <stdexcept>
#include <vector>

namespace
{
    constexpr std::array<CellPosition, 4> Directions =
    {
        CellPosition{  0, -1 },
        CellPosition{  0,  1 },
        CellPosition{ -1,  0 },
        CellPosition{  1,  0 }
    };
}

MazeEndpoints MazeEndpointGenerator::Generate(const Maze& _maze, std::uint32_t _seed, float _minimumDistanceRatio)
{
    if (_maze.GetCellWidth() * _maze.GetCellHeight() < 2)
    {
        throw std::invalid_argument("Maze must contain at least two cells.");
    }

    if (_minimumDistanceRatio < 0.0f || _minimumDistanceRatio > 1.0f)
    {
        throw std::invalid_argument( "Minimum distance ratio must be between 0 and 1." );
    }

    std::mt19937 randomEngine(_seed);

    // Start 후보: 논리 Cell Grid의 외곽
    std::vector<CellPosition> startCandidates;

    for (int y = 0; y < _maze.GetCellHeight(); ++y)
    {
        for (int x = 0; x < _maze.GetCellWidth(); ++x)
        {
            const bool isOuterCell =
                x == 0 ||
                y == 0 ||
                x == _maze.GetCellWidth() - 1 ||
                y == _maze.GetCellHeight() - 1;

            if (!isOuterCell)
            {
                continue;
            }

            startCandidates.push_back({ x, y });
        }
    }

    std::uniform_int_distribution<std::size_t> startDistribution( 0, startCandidates.size() - 1 );

    const CellPosition start = startCandidates[startDistribution(randomEngine)];

    const int cellCount = _maze.GetCellWidth() * _maze.GetCellHeight();

    std::vector<int> distances( static_cast<std::size_t>(cellCount), -1 );

    auto getCellIndex = [&_maze](const CellPosition& _position)
    {
        return static_cast<std::size_t>( _position.y * _maze.GetCellWidth() + _position.x );
    };

    std::queue<CellPosition> queue;

    distances[getCellIndex(start)] = 0;
    queue.push(start);

    int maximumDistance = 0;

    while (!queue.empty())
    {
        const CellPosition current = queue.front();
        queue.pop();

        const int currentDistance = distances[getCellIndex(current)];

        for (const CellPosition& direction : Directions)
        {
            const CellPosition next =
            {
                current.x + direction.x,
                current.y + direction.y
            };

            if (!_maze.IsCellInside(next))
            {
                continue;
            }

            const std::size_t nextIndex = getCellIndex(next);

            if (distances[nextIndex] != -1)
            {
                continue;
            }

            if (!_maze.CanMove(current, next))
            {
                continue;
            }

            const int nextDistance = currentDistance + 1;

            distances[nextIndex] = nextDistance;

            maximumDistance = std::max(maximumDistance, nextDistance);

            queue.push(next);
        }
    }

    if (maximumDistance == 0)
    {
        throw std::runtime_error( "No reachable goal cell was found." );
    }

    const int minimumDistance = std::max( 1, static_cast<int>( std::ceil( static_cast<float>(maximumDistance) * _minimumDistanceRatio ) ) );

    std::vector<CellPosition> goalCandidates;

    for (int y = 0; y < _maze.GetCellHeight(); ++y)
    {
        for (int x = 0; x < _maze.GetCellWidth(); ++x)
        {
            const CellPosition position = { x, y };

            const int distance = distances[getCellIndex(position)];

            if (distance < minimumDistance)
            {
                continue;
            }

            goalCandidates.push_back(position);
        }
    }

    if (goalCandidates.empty())
    {
        throw std::runtime_error( "No valid goal candidate was found." );
    }

    std::uniform_int_distribution<std::size_t> goalDistribution( 0, goalCandidates.size() - 1 );

    const CellPosition goal = goalCandidates[goalDistribution(randomEngine)];

    return MazeEndpoints
    {
        start,
        goal
    };
}