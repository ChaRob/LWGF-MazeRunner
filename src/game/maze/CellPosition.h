#pragma once

struct CellPosition
{
    int x;
    int y;

    bool operator==(const CellPosition&) const = default;
};
