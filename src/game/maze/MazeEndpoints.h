#pragma once

#include "CellPosition.h"

struct MazeEndpoints
{
    CellPosition start;
    CellPosition goal;
    int shortestDistance = 0;
};
