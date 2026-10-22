#include "Stage.h"

#include "../maze/MazeEndpointGenerator.h"
#include "../maze/MazeGenerator.h"

#include <stdexcept>

namespace
{
    constexpr float GoalMinimumDistanceRatio = 0.9f;
    constexpr std::uint32_t EndpointSeedOffset = 0x9E3779B9u;
}

int Stage::GetCellSize(std::uint64_t _stageNumber)
{
    if (_stageNumber == 0)
    {
        throw std::invalid_argument("Stage number must be positive.");
    }

    // Stage 1-10: 5x5, 11-20: 7x7, ..., 101+: 25x25.
    if (_stageNumber >= 101)
    {
        return 25;
    }

    return 5 + static_cast<int>((_stageNumber - 1) / 10) * 2;
}

Stage::Stage(std::uint64_t _number, std::uint32_t _seed)
    : m_number(_number)
    , m_maze(MazeGenerator::Generate(
        GetCellSize(_number),
        GetCellSize(_number),
        _seed))
    , m_endpoints(MazeEndpointGenerator::Generate(
        m_maze,
        _seed ^ EndpointSeedOffset,
        GoalMinimumDistanceRatio))
    , m_player(m_endpoints.start)
{
}

std::uint64_t Stage::GetNumber() const noexcept
{
    return m_number;
}

const Maze& Stage::GetMaze() const noexcept
{
    return m_maze;
}

const MazeEndpoints& Stage::GetEndpoints() const noexcept
{
    return m_endpoints;
}

const Player& Stage::GetPlayer() const noexcept
{
    return m_player;
}

bool Stage::TryMovePlayer(const CellPosition& _direction)
{
    if (!m_player.TryMove(m_maze, _direction))
    {
        return false;
    }

    return m_player.GetPosition() == m_endpoints.goal;
}
