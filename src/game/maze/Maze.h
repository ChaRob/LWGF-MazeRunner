#pragma once
#include <vector>

enum class Direction
{
	Up,
	Down,
	Left,
	Right
};

enum class TileType
{
	Wall,
	Path,
	Start,
	End
};

class Maze
{
public:
	Maze(int width, int height);
	~Maze();

private:
	int cellWidth;
	int cellHeight;
	int tileWidth;
	int tileHeight;

	std::vector<TileType> data;
	std::vector<std::vector<TileType>> map;
}