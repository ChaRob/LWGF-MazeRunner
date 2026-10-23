#include "Stage.h"
#include "../GameTuning.h"

#include "../maze/MazeEndpointGenerator.h"
#include "../maze/MazeGenerator.h"

#include <stdexcept>

namespace
{
    constexpr std::uint32_t EndpointSeedOffset = 0x9E3779B9u;
}

int Stage::GetCellSize(std::uint64_t _stageNumber)
{
    if (_stageNumber == 0)
    {
        throw std::invalid_argument("Stage number must be positive.");
    }

    // Work in tiers before converting to int to avoid overflow for high stage numbers.
    const std::uint64_t tier =
        (_stageNumber - 1) / GameTuning::Maze::StagesPerTier;
    const std::uint64_t maximumTier =
        static_cast<std::uint64_t>(
            (GameTuning::Maze::MaximumCellSize - GameTuning::Maze::InitialCellSize)
            / GameTuning::Maze::CellGrowthPerTier);

    if (tier >= maximumTier)
    {
        return GameTuning::Maze::MaximumCellSize;
    }

    return GameTuning::Maze::InitialCellSize
        + static_cast<int>(tier) * GameTuning::Maze::CellGrowthPerTier;
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
        GameTuning::Maze::GoalMinimumDistanceRatio))
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
