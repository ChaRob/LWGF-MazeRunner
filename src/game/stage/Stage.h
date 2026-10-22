#pragma once

#include "../maze/Maze.h"
#include "../maze/MazeEndpoints.h"
#include "../player/Player.h"

#include <cstdint>

class Stage
{
public:
    Stage(std::uint64_t _number, std::uint32_t _seed);

    std::uint64_t GetNumber() const noexcept;
    const Maze& GetMaze() const noexcept;
    const MazeEndpoints& GetEndpoints() const noexcept;
    const Player& GetPlayer() const noexcept;

    bool TryMovePlayer(const CellPosition& _direction);

    static int GetCellSize(std::uint64_t _stageNumber);

private:
    std::uint64_t m_number;
    Maze m_maze;
    MazeEndpoints m_endpoints;
    Player m_player;
};
