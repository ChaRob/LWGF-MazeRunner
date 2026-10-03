#include "Maze.h"

#include <stdexcept>
#include <cmath>

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

bool Maze::IsCellInside(const CellPosition& _position) const noexcept
{
    return _position.x >= 0 &&
        _position.y >= 0 &&
        _position.x < m_cellWidth &&
        _position.y < m_cellHeight;
}

bool Maze::CanMove(const CellPosition& _from, const CellPosition& _to) const
{
    if (!IsCellInside(_from) || !IsCellInside(_to))
    {
        return false;
    }

    const int deltaX = _to.x - _from.x;
    const int deltaY = _to.y - _from.y;

    // 상하좌우로 한 Cell 이동하는 경우만 허용
    if (std::abs(deltaX) + std::abs(deltaY) != 1)
    {
        return false;
    }

    const int fromTileX = _from.x * 2 + 1;
    const int fromTileY = _from.y * 2 + 1;

    const int toTileX = _to.x * 2 + 1;
    const int toTileY = _to.y * 2 + 1;

    const int wallTileX = (fromTileX + toTileX) / 2;
    const int wallTileY = (fromTileY + toTileY) / 2;

    return GetTile(wallTileX, wallTileY) == TileType::Path;
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