#include "HeldMovementInput.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
    bool IsDirectionHeld(const MovementButtons& _buttons, const CellPosition& _direction)
    {
        if (_direction.x == -1)
        {
            return _buttons.left;
        }
        if (_direction.x == 1)
        {
            return _buttons.right;
        }
        if (_direction.y == 1)
        {
            return _buttons.up;
        }
        if (_direction.y == -1)
        {
            return _buttons.down;
        }
        return false;
    }
}

CellPosition HeldMovementInput::Update(float _deltaTime, const MovementButtons& _buttons)
{
    if (!std::isfinite(_deltaTime) || _deltaTime < 0.0f)
    {
        throw std::invalid_argument("Delta time must be finite and nonnegative.");
    }

    if (m_waitingForRelease)
    {
        if (!_buttons.AnyDown())
        {
            m_waitingForRelease = false;
        }
        m_previous = _buttons;
        return { 0, 0 };
    }

    // A newly pressed direction wins over a key already being held.
    CellPosition direction = { 0, 0 };
    if (_buttons.up && !m_previous.up)
    {
        direction = { 0, 1 };
    }
    else if (_buttons.down && !m_previous.down)
    {
        direction = { 0, -1 };
    }
    else if (_buttons.left && !m_previous.left)
    {
        direction = { -1, 0 };
    }
    else if (_buttons.right && !m_previous.right)
    {
        direction = { 1, 0 };
    }
    else if (IsDirectionHeld(_buttons, m_activeDirection))
    {
        direction = m_activeDirection;
    }
    else if (_buttons.up)
    {
        direction = { 0, 1 };
    }
    else if (_buttons.down)
    {
        direction = { 0, -1 };
    }
    else if (_buttons.left)
    {
        direction = { -1, 0 };
    }
    else if (_buttons.right)
    {
        direction = { 1, 0 };
    }

    m_previous = _buttons;

    // A single movement cooldown applies across axes and direction changes.
    // Keep advancing it even when no key is held, so a fresh press after
    // sufficient idle time can move immediately without bypassing the interval.
    m_elapsedTime = std::min(RepeatInterval, m_elapsedTime + _deltaTime);
    m_activeDirection = direction;

    if (direction == CellPosition{ 0, 0 })
    {
        return { 0, 0 };
    }

    if (m_elapsedTime >= RepeatInterval)
    {
        // At most one grid step per frame; discard long-frame backlog.
        m_elapsedTime = 0.0f;
        return direction;
    }

    return { 0, 0 };
}

void HeldMovementInput::RequireRelease() noexcept
{
    m_previous = {};
    m_activeDirection = { 0, 0 };
    m_elapsedTime = RepeatInterval;
    m_waitingForRelease = true;
}
