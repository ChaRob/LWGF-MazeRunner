#include "Stage.h"
#include "../GameTuning.h"

#include "../maze/MazeEndpointGenerator.h"
#include "../maze/MazeGenerator.h"

#include <algorithm>
#include <stdexcept>
#include <random>
#include <vector>

namespace
{
    constexpr std::uint32_t EndpointSeedOffset = 0x9E3779B9u;
    constexpr std::uint32_t TimeOrbSeedOffset = 0xD1B54A35u;
    constexpr std::uint32_t RecallSeedOffset = 0xBF58476Du;
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
    // Independent rolls keep the existing maze, endpoints and Time Orb
    // distribution unchanged when Recall is introduced.
    std::mt19937 orbRandom(_seed ^ TimeOrbSeedOffset);
    std::bernoulli_distribution orbSpawn(
        GameTuning::TimeOrb::GetSpawnChance(m_number));
    const bool spawnOrb = orbSpawn(orbRandom);

    std::mt19937 recallRandom(_seed ^ RecallSeedOffset);
    std::bernoulli_distribution recallSpawn(
        GameTuning::Recall::GetSpawnChance(m_number));
    const bool spawnRecall = recallSpawn(recallRandom);

    if (!spawnOrb && !spawnRecall)
    {
        return;
    }

    // Every logical Cell is traversable; do not favor the shortest path.
    std::vector<CellPosition> candidates;
    candidates.reserve(static_cast<std::size_t>(
        m_maze.GetCellWidth() * m_maze.GetCellHeight() - 2));
    for (int y = 0; y < m_maze.GetCellHeight(); ++y)
    {
        for (int x = 0; x < m_maze.GetCellWidth(); ++x)
        {
            const CellPosition cell{ x, y };
            if (cell != m_endpoints.start && cell != m_endpoints.goal)
            {
                candidates.push_back(cell);
            }
        }
    }

    if (spawnOrb && !candidates.empty())
    {
        std::uniform_int_distribution<std::size_t> distribution(
            0, candidates.size() - 1);
        m_timeOrbPosition = candidates[distribution(orbRandom)];
    }

    if (spawnRecall)
    {
        // Recall and the Time Orb have independent spawn chances but cannot
        // occupy the same Cell. Reroute Recall only; never move the Time Orb.
        if (m_timeOrbPosition)
        {
            const auto it = std::find(candidates.begin(), candidates.end(),
                *m_timeOrbPosition);
            if (it != candidates.end())
            {
                candidates.erase(it);
            }
        }
        if (!candidates.empty())
        {
            std::uniform_int_distribution<std::size_t> distribution(
                0, candidates.size() - 1);
            m_recallPosition = candidates[distribution(recallRandom)];
        }
    }
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

const std::optional<CellPosition>& Stage::GetTimeOrbPosition() const noexcept
{
    return m_timeOrbPosition;
}

const std::optional<CellPosition>& Stage::GetRecallPosition() const noexcept
{
    return m_recallPosition;
}

bool Stage::TryCollectRecallAtPlayerPosition() noexcept
{
    if (!m_recallPosition || m_player.GetPosition() != *m_recallPosition)
    {
        return false;
    }

    m_recallPosition.reset();
    return true;
}

bool Stage::RecallPlayerToStart() noexcept
{
    if (m_player.GetPosition() == m_endpoints.start)
    {
        return false;
    }

    m_player.ResetPosition(m_endpoints.start);
    m_timeOrbCollectedOnLastMove = false;
    return true;
}

bool Stage::WasTimeOrbCollectedOnLastMove() const noexcept
{
    return m_timeOrbCollectedOnLastMove;
}

bool Stage::TryMovePlayer(const CellPosition& _direction)
{
    m_timeOrbCollectedOnLastMove = false;
    if (!m_player.TryMove(m_maze, _direction))
    {
        return false;
    }

    if (m_timeOrbPosition && m_player.GetPosition() == *m_timeOrbPosition)
    {
        m_timeOrbPosition.reset(); // Exactly one pickup per stage.
        m_timeOrbCollectedOnLastMove = true;
    }

    return m_player.GetPosition() == m_endpoints.goal;
}
