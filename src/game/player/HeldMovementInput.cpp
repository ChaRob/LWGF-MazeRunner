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

    bool IsDirectionAvailable(const MovementAvailability& _available,
        const CellPosition& _direction)
    {
        if (_direction.x == -1) return _available.left;
        if (_direction.x == 1) return _available.right;
        if (_direction.y == 1) return _available.up;
        if (_direction.y == -1) return _available.down;
        return false;
    }
}

CellPosition HeldMovementInput::Update(float _deltaTime,
    const MovementButtons& _buttons, const MovementAvailability& _available)
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

    // Remember the most recently pressed direction until that key is released.
    // A turn pressed before the junction stays pending while we move forward.
    if (_buttons.up && !m_previous.up)
    {
        m_preferredDirection = { 0, 1 };
    }
    else if (_buttons.down && !m_previous.down)
    {
        m_preferredDirection = { 0, -1 };
    }
    else if (_buttons.left && !m_previous.left)
    {
        m_preferredDirection = { -1, 0 };
    }
    else if (_buttons.right && !m_previous.right)
    {
        m_preferredDirection = { 1, 0 };
    }

    if (!IsDirectionHeld(_buttons, m_preferredDirection))
    {
        m_preferredDirection = { 0, 0 };
    }
    m_previous = _buttons;

    m_elapsedTime = std::min(RepeatInterval, m_elapsedTime + _deltaTime);

    CellPosition direction = { 0, 0 };
    if (IsDirectionHeld(_buttons, m_preferredDirection)
        && IsDirectionAvailable(_available, m_preferredDirection))
    {
        direction = m_preferredDirection;
    }
    else if (IsDirectionHeld(_buttons, m_activeDirection)
        && IsDirectionAvailable(_available, m_activeDirection))
    {
        // Blocked early corner presses must not stall forward movement.
        direction = m_activeDirection;
    }
    else if (_buttons.up && _available.up)
    {
        direction = { 0, 1 };
    }
    else if (_buttons.down && _available.down)
    {
        direction = { 0, -1 };
    }
    else if (_buttons.left && _available.left)
    {
        direction = { -1, 0 };
    }
    else if (_buttons.right && _available.right)
    {
        direction = { 1, 0 };
    }

    if (direction == CellPosition{ 0, 0 } || m_elapsedTime < RepeatInterval)
    {
        return { 0, 0 };
    }

    // One logical step per interval and at most one step per frame.
    m_elapsedTime = 0.0f;
    m_activeDirection = direction;
    return direction;
}

void HeldMovementInput::RequireRelease() noexcept
{
    m_previous = {};
    m_preferredDirection = { 0, 0 };
    m_activeDirection = { 0, 0 };
    m_elapsedTime = RepeatInterval;
    m_waitingForRelease = true;
}
