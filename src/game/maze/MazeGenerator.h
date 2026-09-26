#pragma once

#include "Maze.h"

#include <cstdint>

class MazeGenerator
{
public:
    static Maze Generate(int _cellWidth, int _cellHeight, std::uint32_t _seed);
};