#include "GameSession.h"
#include "GameTuning.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

GameSession::GameSession(std::uint32_t _seed)
    : m_random(_seed)
    , m_stage(1, m_random())
    , m_remainingTime(GameTuning::Time::InitialSeconds)
{
    std::cout << "Stage 1 started ("
              << m_stage.GetMaze().GetCellWidth() << "x"
              << m_stage.GetMaze().GetCellHeight() << ")\n";
}

void GameSession::Update(float _deltaTime)
{
    if (m_status != GameStatus::Playing)
    {
        return;
    }

    if (!std::isfinite(_deltaTime) || _deltaTime < 0.0f)
    {
        throw std::invalid_argument("Delta time must be finite and nonnegative.");
    }

    m_stageElapsedTime += _deltaTime;
    m_remainingTime = std::max(0.0f, m_remainingTime - _deltaTime);

    if (m_remainingTime <= 0.0f)
    {
        m_status = GameStatus::GameOver;
        std::cout << "Game Over! Completed stages: " << m_completedStages
                  << ", Score: " << m_score << '\n';
    }
}

void GameSession::TryMove(const CellPosition& _direction)
{
    if (m_status != GameStatus::Playing)
    {
        return;
    }

    if (!m_stage.TryMovePlayer(_direction))
    {
        return;
    }

    const int distance = m_stage.GetEndpoints().shortestDistance;
    const float timeBonus = static_cast<float>(distance) * GameTuning::Time::ClearBonusSecondsPerCell;

    // Provisional scoring: reward progress and actual maze path length.
    const float speedScoreBaseline =
        static_cast<float>(distance) * GameTuning::Score::SpeedBaselineSecondsPerCell;
    const float savedSeconds = std::max(0.0f, speedScoreBaseline - m_stageElapsedTime);
    const std::uint64_t speedPoints =
        static_cast<std::uint64_t>(std::floor(savedSeconds * GameTuning::Score::SpeedPointsPerSecondSaved));
    const std::uint64_t points =
        GameTuning::Score::BasePointsPerStage +
        static_cast<std::uint64_t>(distance) * GameTuning::Score::DistancePointsPerCell +
        speedPoints;

    if (m_score > std::numeric_limits<std::uint64_t>::max() - points)
    {
        m_score = std::numeric_limits<std::uint64_t>::max();
    }
    else
    {
        m_score += points;
    }

    ++m_completedStages;
    m_remainingTime = std::min(GameTuning::Time::MaximumSeconds, m_remainingTime + timeBonus);

    std::cout << "Stage " << m_stage.GetNumber()
              << " clear! Distance: " << distance
              << ", Elapsed: " << m_stageElapsedTime
              << "s, Bonus: +" << timeBonus
              << "s, Score: " << m_score << '\n';

    if (m_stage.GetNumber() == std::numeric_limits<std::uint64_t>::max())
    {
        m_status = GameStatus::GameOver;
        return;
    }

    m_stage = Stage(m_stage.GetNumber() + 1, m_random());
    m_stageElapsedTime = 0.0f;

    std::cout << "Stage " << m_stage.GetNumber()
              << " started (" << m_stage.GetMaze().GetCellWidth()
              << "x" << m_stage.GetMaze().GetCellHeight() << ")\n";
}

const Stage& GameSession::GetStage() const noexcept
{
    return m_stage;
}

GameStatus GameSession::GetStatus() const noexcept
{
    return m_status;
}

float GameSession::GetRemainingTime() const noexcept
{
    return m_remainingTime;
}

float GameSession::GetInitialTime() const noexcept
{
    return GameTuning::Time::InitialSeconds;
}

std::uint64_t GameSession::GetScore() const noexcept
{
    return m_score;
}

std::uint64_t GameSession::GetCompletedStages() const noexcept
{
    return m_completedStages;
}
