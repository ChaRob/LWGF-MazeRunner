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
    , m_stageEntryTime(GameTuning::Time::InitialSeconds)
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

    m_remainingTime = std::max(0.0f, m_remainingTime - _deltaTime);

    if (m_remainingTime <= 0.0f)
    {
        m_status = GameStatus::GameOver;
        std::cout << "Game Over! Completed stages: " << m_completedStages
                  << '\n';
    }
}

void GameSession::TryMove(const CellPosition& _direction)
{
    if (m_status != GameStatus::Playing)
    {
        return;
    }

    m_timeOrbCollectedOnLastMove = false;
    m_recallCollectedOnLastMove = false;
    m_lastTimeOrbGrantedSeconds = 0.0f;

    const CellPosition priorPosition = m_stage.GetPlayer().GetPosition();
    const bool stageCleared = m_stage.TryMovePlayer(_direction);
    const CellPosition currentPosition = m_stage.GetPlayer().GetPosition();
    if (currentPosition != priorPosition && m_stage.GetRecallPosition()
        && currentPosition == *m_stage.GetRecallPosition()
        && m_inventory.TryAdd(ActiveItemType::Recall))
    {
        m_recallCollectedOnLastMove = m_stage.TryCollectRecallAtPlayerPosition();
        std::cout << "Recall collected! Inventory: "
                  << m_inventory.GetCount(ActiveItemType::Recall) << '\\n';
    }
    if (m_stage.WasTimeOrbCollectedOnLastMove())
    {
        m_timeOrbCollectedOnLastMove = true;
        const float previousTime = m_remainingTime;
        m_remainingTime = std::min(GameTuning::Time::MaximumSeconds,
            m_remainingTime + GameTuning::TimeOrb::RecoverySeconds);
        m_lastTimeOrbGrantedSeconds = m_remainingTime - previousTime;
        std::cout << "Time Orb collected! Granted: +"
                  << m_lastTimeOrbGrantedSeconds << "s\n";
    }

    if (!stageCleared)
    {
        return;
    }

    const int distance = m_stage.GetEndpoints().shortestDistance;
    const float timeBonus = static_cast<float>(distance) * GameTuning::Time::ClearBonusSecondsPerCell;

    ++m_completedStages;
    m_remainingTime = std::min(GameTuning::Time::MaximumSeconds, m_remainingTime + timeBonus);

    std::cout << "Stage " << m_stage.GetNumber()
              << " clear! Distance: " << distance
              << ", Bonus: +" << timeBonus << "s\n";

    if (m_stage.GetNumber() == std::numeric_limits<std::uint64_t>::max())
    {
        m_status = GameStatus::GameOver;
        return;
    }

    m_stage = Stage(m_stage.GetNumber() + 1, m_random());
    m_stageEntryTime = m_remainingTime;

    std::cout << "Stage " << m_stage.GetNumber()
              << " started (" << m_stage.GetMaze().GetCellWidth()
              << "x" << m_stage.GetMaze().GetCellHeight() << ")\n";
}

bool GameSession::TryUseItem(ActiveItemType _item)
{
    if (m_status != GameStatus::Playing || _item != ActiveItemType::Recall
        || m_inventory.GetCount(_item) == 0
        || !m_stage.RecallPlayerToStart())
    {
        return false;
    }

    // Only position and time rewind: maze, collected pickups and consumed
    // inventory are NOT reset. No benefit if the current clock is higher.
    m_inventory.TryConsume(_item);
    m_remainingTime = std::min(GameTuning::Time::MaximumSeconds,
        std::max(m_remainingTime, m_stageEntryTime));
    m_timeOrbCollectedOnLastMove = false;
    m_recallCollectedOnLastMove = false;
    m_lastTimeOrbGrantedSeconds = 0.0f;
    std::cout << "Recall used! Time: " << m_remainingTime << "s\n";
    return true;
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

std::uint64_t GameSession::GetCompletedStages() const noexcept
{
    return m_completedStages;
}

bool GameSession::WasTimeOrbCollectedOnLastMove() const noexcept
{
    return m_timeOrbCollectedOnLastMove;
}

float GameSession::GetLastTimeOrbGrantedSeconds() const noexcept
{
    return m_lastTimeOrbGrantedSeconds;
}

bool GameSession::WasRecallCollectedOnLastMove() const noexcept
{
    return m_recallCollectedOnLastMove;
}

const ActiveItemInventory& GameSession::GetInventory() const noexcept
{
    return m_inventory;
}

float GameSession::GetStageEntryTime() const noexcept
{
    return m_stageEntryTime;
}
