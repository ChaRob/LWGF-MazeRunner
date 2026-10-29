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
    RecordStageSpawns();
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

    // Count only active gameplay time. If a long frame crosses zero,
    // the portion beyond the countdown deadline is not playable time.
    m_statistics.AddPlayingSeconds(
        static_cast<double>(std::min(_deltaTime, m_remainingTime)));
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

    ResetPickupEvents();

    const CellPosition priorPosition = m_stage.GetPlayer().GetPosition();
    const bool stageCleared = m_stage.TryMovePlayer(_direction);
    if (m_stage.GetPlayer().GetPosition() != priorPosition)
    {
        m_statistics.RecordSuccessfulMove();
        CollectPickupsAtPlayerPosition();
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
    RecordStageSpawns();

    std::cout << "Stage " << m_stage.GetNumber()
              << " started (" << m_stage.GetMaze().GetCellWidth()
              << "x" << m_stage.GetMaze().GetCellHeight() << ")\n";
}

void GameSession::RecordStageSpawns() noexcept
{
    // Count each generated stage once, including the final stage that kills
    // the player. Never count a new item just because it remains on the map.
    if (m_stage.GetTimeOrbPosition())
        m_statistics.RecordSpawn(RunItemType::TimeOrb);
    if (m_stage.GetRecallPosition())
        m_statistics.RecordSpawn(RunItemType::Recall);
    if (m_stage.GetWallBreakPosition())
        m_statistics.RecordSpawn(RunItemType::WallBreak);
    if (m_stage.GetTeleportPosition())
        m_statistics.RecordSpawn(RunItemType::Teleport);
}

void GameSession::ResetPickupEvents() noexcept
{
    m_timeOrbCollectedOnLastMove = false;
    m_recallCollectedOnLastMove = false;
    m_wallBreakCollectedOnLastMove = false;
    m_teleportCollectedOnLastMove = false;
    m_lastTimeOrbGrantedSeconds = 0.0f;
}

void GameSession::CollectPickupsAtPlayerPosition()
{
    const CellPosition player = m_stage.GetPlayer().GetPosition();
    if (m_stage.GetRecallPosition() && player == *m_stage.GetRecallPosition()
        && m_inventory.TryAdd(ActiveItemType::Recall))
    {
        m_recallCollectedOnLastMove = m_stage.TryCollectRecallAtPlayerPosition();
        if (m_recallCollectedOnLastMove)
            m_statistics.RecordPickup(RunItemType::Recall);
        std::cout << "Recall collected! Inventory: "
                  << m_inventory.GetCount(ActiveItemType::Recall) << '\\n';
    }
    if (m_stage.GetWallBreakPosition() && player == *m_stage.GetWallBreakPosition()
        && m_inventory.TryAdd(ActiveItemType::WallBreak))
    {
        m_wallBreakCollectedOnLastMove = m_stage.TryCollectWallBreakAtPlayerPosition();
        if (m_wallBreakCollectedOnLastMove)
            m_statistics.RecordPickup(RunItemType::WallBreak);
        std::cout << "Wall Break collected! Inventory: "
                  << m_inventory.GetCount(ActiveItemType::WallBreak) << '\n';
    }
    if (m_stage.GetTeleportPosition() && player == *m_stage.GetTeleportPosition()
        && m_inventory.TryAdd(ActiveItemType::Teleport))
    {
        m_teleportCollectedOnLastMove = m_stage.TryCollectTeleportAtPlayerPosition();
        if (m_teleportCollectedOnLastMove)
            m_statistics.RecordPickup(RunItemType::Teleport);
        std::cout << "Teleport collected! Inventory: "
                  << m_inventory.GetCount(ActiveItemType::Teleport) << '\n';
    }
    if (m_stage.WasTimeOrbCollectedOnLastMove())
    {
        m_timeOrbCollectedOnLastMove = true;
        m_statistics.RecordPickup(RunItemType::TimeOrb);
        const float before = m_remainingTime;
        m_remainingTime = std::min(GameTuning::Time::MaximumSeconds,
            m_remainingTime + GameTuning::TimeOrb::RecoverySeconds);
        m_lastTimeOrbGrantedSeconds = m_remainingTime - before;
        std::cout << "Time Orb collected! Granted: +"
                  << m_lastTimeOrbGrantedSeconds << "s\n";
    }
}

bool GameSession::TryTeleport(const CellPosition& _destination)
{
    if (m_status != GameStatus::Playing
        || m_inventory.GetCount(ActiveItemType::Teleport) == 0
        || !m_stage.TryTeleportPlayer(_destination))
    {
        return false; // No item spent on invalid destination.
    }

    m_inventory.TryConsume(ActiveItemType::Teleport);
    m_statistics.RecordUse(RunItemType::Teleport);
    ResetPickupEvents();
    CollectPickupsAtPlayerPosition();
    std::cout << "Teleported! Inventory: "
              << m_inventory.GetCount(ActiveItemType::Teleport) << '\n';
    return true;
}

bool GameSession::TryUseItem(ActiveItemType _item,
    const CellPosition& _direction)
{
    if (m_status != GameStatus::Playing || m_inventory.GetCount(_item) == 0)
    {
        return false;
    }

    if (_item == ActiveItemType::Recall)
    {
        if (!m_stage.RecallPlayerToStart())
        {
            return false;
        }

        // Rewind only position and time, never consumed pickups or wall edits.
        m_inventory.TryConsume(_item);
        m_statistics.RecordUse(RunItemType::Recall);
        m_remainingTime = std::min(GameTuning::Time::MaximumSeconds,
            std::max(m_remainingTime, m_stageEntryTime));
        ResetPickupEvents();
        std::cout << "Recall used! Time: " << m_remainingTime << "s\n";
        return true;
    }

    if (_item == ActiveItemType::WallBreak)
    {
        if (!m_stage.TryBreakWall(_direction))
        {
            return false; // Invalid direction, open corridor or perimeter: no cost.
        }

        m_inventory.TryConsume(_item);
        m_statistics.RecordUse(RunItemType::WallBreak);
        std::cout << "Wall broken! Inventory: "
                  << m_inventory.GetCount(ActiveItemType::WallBreak) << '\n';
        return true;
    }

    return false; // Teleport uses TryTeleport(destination).
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

const RunStatistics& GameSession::GetStatistics() const noexcept
{
    return m_statistics;
}

float GameSession::GetStageEntryTime() const noexcept
{
    return m_stageEntryTime;
}

bool GameSession::WasWallBreakCollectedOnLastMove() const noexcept
{
    return m_wallBreakCollectedOnLastMove;
}

bool GameSession::WasTeleportCollectedOnLastMove() const noexcept
{
    return m_teleportCollectedOnLastMove;
}
