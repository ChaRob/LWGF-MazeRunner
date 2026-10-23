#pragma once

#include "../GameTuning.h"
#include "../maze/CellPosition.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

struct VisualCellPosition
{
    float x = 0.0f;
    float y = 0.0f;
};

// Presentation only: move between successive adjacent cell centers.
// The logical position and wall / goal checks remain discrete.
class PlayerMovementVisual
{
public:
    void Reset(const CellPosition& _position) noexcept
    {
        m_from = _position;
        m_to = _position;
        m_elapsedSeconds = GameTuning::Movement::RepeatIntervalSeconds;
        m_moving = false;
    }

    void BeginMove(const CellPosition& _from, const CellPosition& _to) noexcept
    {
        m_from = _from;
        m_to = _to;
        m_elapsedSeconds = 0.0f;
        m_moving = true;
    }

    void Update(float _deltaTime)
    {
        if (!std::isfinite(_deltaTime) || _deltaTime < 0.0f)
        {
            throw std::invalid_argument("Delta time must be finite and nonnegative.");
        }

        if (!m_moving)
        {
            return;
        }

        m_elapsedSeconds = std::min(
            GameTuning::Movement::RepeatIntervalSeconds,
            m_elapsedSeconds + _deltaTime);
        if (m_elapsedSeconds >= GameTuning::Movement::RepeatIntervalSeconds)
        {
            m_moving = false;
        }
    }

    VisualCellPosition GetPosition() const noexcept
    {
        if (!m_moving)
        {
            return { static_cast<float>(m_to.x), static_cast<float>(m_to.y) };
        }

        const float progress = std::clamp(
            m_elapsedSeconds / GameTuning::Movement::RepeatIntervalSeconds,
            0.0f, 1.0f);
        return {
            static_cast<float>(m_from.x) + static_cast<float>(m_to.x - m_from.x) * progress,
            static_cast<float>(m_from.y) + static_cast<float>(m_to.y - m_from.y) * progress
        };
    }

private:
    CellPosition m_from = {};
    CellPosition m_to = {};
    float m_elapsedSeconds = GameTuning::Movement::RepeatIntervalSeconds;
    bool m_moving = false;
};
