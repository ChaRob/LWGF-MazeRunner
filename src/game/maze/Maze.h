#pragma once
#include <vector>

enum class TileType
{
	Wall,
	Path,
};

class MazeGenerator;

class Maze
{
public:
	Maze(int _cellWidth, int _cellHeight);

	int GetCellWidth() const noexcept;
	int GetCellHeight() const noexcept;

	int GetTileWidth() const noexcept;
	int GetTileHeight() const noexcept;

	TileType GetTile(int _x, int _y) const;

	bool IsTileInside(int _x, int _y) const noexcept;

private:
	friend class MazeGenerator;

	void SetTile(int _x, int _y, TileType _type);

	std::size_t GetIndex(int _x, int _y) const;

public:


private:
	int m_cellWidth;
	int m_cellHeight;
	int m_tileWidth;
	int m_tileHeight;

	std::vector<TileType> m_tiles;
};