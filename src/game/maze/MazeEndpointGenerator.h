#pragma once

#include "Maze.h"
#include "MazeEndpoints.h"

#include <cstdint>

class MazeEndpointGenerator
{
public:
    static MazeEndpoints Generate(const Maze& _maze, std::uint32_t _seed, float _minimumDistanceRatio = 0.5f);
};