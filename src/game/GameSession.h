#pragma once

#include "stage/Stage.h"

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

    const Stage& GetStage() const noexcept;
    GameStatus GetStatus() const noexcept;
    float GetRemainingTime() const noexcept;
    float GetInitialTime() const noexcept;
    std::uint64_t GetCompletedStages() const noexcept;

private:
    std::mt19937 m_random;
    Stage m_stage;
    GameStatus m_status = GameStatus::Playing;
    float m_remainingTime = 30.0f;
    std::uint64_t m_completedStages = 0;
};
