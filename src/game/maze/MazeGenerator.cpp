#include "MazeGenerator.h"

#include <array>
#include <cstdint>
#include <random>
#include <vector>

namespace
{
    struct CellPosition
    {
        int x;
        int y;
    };

    constexpr std::array<CellPosition, 4> Directions =
    {
        CellPosition{  0, -1 },
        CellPosition{  0,  1 },
        CellPosition{ -1,  0 },
        CellPosition{  1,  0 }
    };
}

Maze MazeGenerator::Generate(
    int _cellWidth,
    int _cellHeight,
    std::uint32_t _seed)
{
    Maze maze(_cellWidth, _cellHeight);

    std::mt19937 randomEngine(_seed);

    std::uniform_int_distribution<int> startXDistribution(
        0,
        _cellWidth - 1
    );

    std::uniform_int_distribution<int> startYDistribution(
        0,
        _cellHeight - 1
    );

    std::vector<std::uint8_t> visited(
        static_cast<std::size_t>(_cellWidth * _cellHeight),
        0
    );

    auto getCellIndex = [_cellWidth](int _x, int _y)
        {
            return static_cast<std::size_t>(_y * _cellWidth + _x);
        };

    const CellPosition startCell =
    {
        startXDistribution(randomEngine),
        startYDistribution(randomEngine)
    };

    std::vector<CellPosition> stack;
    stack.reserve(
        static_cast<std::size_t>(_cellWidth * _cellHeight)
    );

    stack.push_back(startCell);

    visited[getCellIndex(startCell.x, startCell.y)] = 1;

    while (!stack.empty())
    {
        const CellPosition currentCell = stack.back();

        std::array<CellPosition, 4> candidates;
        int candidateCount = 0;

        for (const CellPosition& direction : Directions)
        {
            const CellPosition nextCell =
            {
                currentCell.x + direction.x,
                currentCell.y + direction.y
            };

            if (nextCell.x < 0 ||
                nextCell.y < 0 ||
                nextCell.x >= _cellWidth ||
                nextCell.y >= _cellHeight)
            {
                continue;
            }

            if (visited[getCellIndex(nextCell.x, nextCell.y)] != 0)
            {
                continue;
            }

            candidates[candidateCount] = nextCell;
            ++candidateCount;
        }

        if (candidateCount == 0)
        {
            stack.pop_back();
            continue;
        }

        std::uniform_int_distribution<int> candidateDistribution(
            0,
            candidateCount - 1
        );

        const CellPosition nextCell = candidates[candidateDistribution(randomEngine)];

        const int currentTileX = currentCell.x * 2 + 1;
        const int currentTileY = currentCell.y * 2 + 1;

        const int nextTileX = nextCell.x * 2 + 1;
        const int nextTileY = nextCell.y * 2 + 1;

        const int wallTileX = (currentTileX + nextTileX) / 2;
        const int wallTileY = (currentTileY + nextTileY) / 2;

        maze.SetTile(
            wallTileX,
            wallTileY,
            TileType::Path
        );

        visited[getCellIndex(nextCell.x, nextCell.y)] = 1;

        stack.push_back(nextCell);
    }

    return maze;
}