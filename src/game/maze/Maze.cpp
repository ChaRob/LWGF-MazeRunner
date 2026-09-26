#include "Maze.h"

#include <stdexcept>

Maze::Maze(int _cellWidth, int _cellHeight)
    : m_cellWidth(_cellWidth)
    , m_cellHeight(_cellHeight)
    , m_tileWidth(_cellWidth * 2 + 1)
    , m_tileHeight(_cellHeight * 2 + 1)
{
    if (_cellWidth <= 0 || _cellHeight <= 0)
    {
        throw std::invalid_argument("Maze size must be greater than zero.");
    }

    m_tiles.resize(
        static_cast<std::size_t>(m_tileWidth * m_tileHeight),
        TileType::Wall
    );

    for (int y = 0; y < m_cellHeight; ++y)
    {
        for (int x = 0; x < m_cellWidth; ++x)
        {
            const int tileX = x * 2 + 1;
            const int tileY = y * 2 + 1;

            SetTile(tileX, tileY, TileType::Path);
        }
    }
}

int Maze::GetCellWidth() const noexcept
{
    return m_cellWidth;
}

int Maze::GetCellHeight() const noexcept
{
    return m_cellHeight;
}

int Maze::GetTileWidth() const noexcept
{
    return m_tileWidth;
}

int Maze::GetTileHeight() const noexcept
{
    return m_tileHeight;
}

TileType Maze::GetTile(int _x, int _y) const
{
    if (!IsTileInside(_x, _y))
    {
        throw std::out_of_range("Tile position is outside the maze.");
    }

    return m_tiles[GetIndex(_x, _y)];
}

bool Maze::IsTileInside(int _x, int _y) const noexcept
{
    return _x >= 0 &&
        _y >= 0 &&
        _x < m_tileWidth &&
        _y < m_tileHeight;
}

void Maze::SetTile(int _x, int _y, TileType _type)
{
    if (!IsTileInside(_x, _y))
    {
        throw std::out_of_range("Tile position is outside the maze.");
    }

    m_tiles[GetIndex(_x, _y)] = _type;
}

std::size_t Maze::GetIndex(int _x, int _y) const
{
    return static_cast<std::size_t>(_y * m_tileWidth + _x);
}