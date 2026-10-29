#pragma once

#include "stage/Stage.h"
#include "items/ActiveItemInventory.h"
#include "RunStatistics.h"

#include <cstdint>
#include <random>

enum class GameStatus
{
    Playing,
    GameOver
};

class GameSession
{
public:
    explicit GameSession(std::uint32_t _seed);

    void Update(float _deltaTime);
    void TryMove(const CellPosition& _direction);
    bool TryTeleport(const CellPosition& _destination);
    bool TryUseItem(ActiveItemType _item,
        const CellPosition& _direction = { 0, 0 });

    const Stage& GetStage() const noexcept;
    GameStatus GetStatus() const noexcept;
    float GetRemainingTime() const noexcept;
    float GetInitialTime() const noexcept;
    std::uint64_t GetCompletedStages() const noexcept;
    bool WasTimeOrbCollectedOnLastMove() const noexcept;
    float GetLastTimeOrbGrantedSeconds() const noexcept;
    bool WasRecallCollectedOnLastMove() const noexcept;
    bool WasWallBreakCollectedOnLastMove() const noexcept;
    bool WasTeleportCollectedOnLastMove() const noexcept;
    const ActiveItemInventory& GetInventory() const noexcept;
    const RunStatistics& GetStatistics() const noexcept;
    float GetStageEntryTime() const noexcept;

private:
    void ResetPickupEvents() noexcept;
    void CollectPickupsAtPlayerPosition();
    void RecordStageSpawns() noexcept;

    std::mt19937 m_random;
    Stage m_stage;
    GameStatus m_status = GameStatus::Playing;
    float m_remainingTime = 30.0f;
    float m_stageEntryTime = 30.0f;
    ActiveItemInventory m_inventory;
    RunStatistics m_statistics;
    std::uint64_t m_completedStages = 0;
    bool m_timeOrbCollectedOnLastMove = false;
    bool m_recallCollectedOnLastMove = false;
    bool m_wallBreakCollectedOnLastMove = false;
    bool m_teleportCollectedOnLastMove = false;
    float m_lastTimeOrbGrantedSeconds = 0.0f;
};
