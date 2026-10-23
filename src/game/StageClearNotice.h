#pragma once

#include "GameTuning.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

struct StageClearReward
{
    std::uint64_t completedStage = 0;
    float grantedTimeSeconds = 0.0f;
    bool newPersonalBest = false;
};

// Presentation-only notice. It never changes the game state or gameplay clock.
class StageClearNotice
{
public:
    void Show(const StageClearReward& _reward) noexcept
    {
        m_reward = _reward;
        m_elapsedSeconds = 0.0f;
        m_visible = true;
    }

    void Update(float _deltaTime)
    {
        if (!std::isfinite(_deltaTime) || _deltaTime < 0.0f)
        {
            throw std::invalid_argument("Delta time must be finite and nonnegative.");
        }

        if (!m_visible)
        {
            return;
        }

        m_elapsedSeconds += _deltaTime;
        if (m_elapsedSeconds >= GameTuning::Transition::ClearNoticeSeconds)
        {
            Clear();
        }
    }

    void Clear() noexcept
    {
        m_visible = false;
        m_elapsedSeconds = 0.0f;
    }

    bool IsVisible() const noexcept
    {
        return m_visible;
    }

    float GetOpacity() const noexcept
    {
        if (!m_visible)
        {
            return 0.0f;
        }

        // Stay readable at first, then fade away during the last 0.2 seconds.
        constexpr float FadeOutSeconds = 0.2f;
        const float remainingSeconds =
            GameTuning::Transition::ClearNoticeSeconds - m_elapsedSeconds;
        return std::clamp(remainingSeconds / FadeOutSeconds, 0.0f, 1.0f);
    }

    const StageClearReward& GetReward() const noexcept
    {
        return m_reward;
    }

private:
    StageClearReward m_reward;
    float m_elapsedSeconds = 0.0f;
    bool m_visible = false;
};
