#pragma once

#include "stage/Stage.h"
#include "items/ActiveItemInventory.h"

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
    bool TryUseItem(ActiveItemType _item);

    const Stage& GetStage() const noexcept;
    GameStatus GetStatus() const noexcept;
    float GetRemainingTime() const noexcept;
    float GetInitialTime() const noexcept;
    std::uint64_t GetCompletedStages() const noexcept;
    bool WasTimeOrbCollectedOnLastMove() const noexcept;
    float GetLastTimeOrbGrantedSeconds() const noexcept;
    bool WasRecallCollectedOnLastMove() const noexcept;
    const ActiveItemInventory& GetInventory() const noexcept;
    float GetStageEntryTime() const noexcept;

private:
    std::mt19937 m_random;
    Stage m_stage;
    GameStatus m_status = GameStatus::Playing;
    float m_remainingTime = 30.0f;
    float m_stageEntryTime = 30.0f;
    ActiveItemInventory m_inventory;
    std::uint64_t m_completedStages = 0;
    bool m_timeOrbCollectedOnLastMove = false;
    bool m_recallCollectedOnLastMove = false;
    float m_lastTimeOrbGrantedSeconds = 0.0f;
};
