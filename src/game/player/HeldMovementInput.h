#pragma once

#include "../maze/CellPosition.h"

struct MovementButtons
{
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;

    bool AnyDown() const noexcept
    {
        return up || down || left || right;
    }
};

// Converts held direction keys to discrete grid steps without changing Player physics.
class HeldMovementInput
{
public:
    // First step is immediate. Repeats occur every 0.12 seconds while held.
    CellPosition Update(float _deltaTime, const MovementButtons& _buttons);

    // Ignore all held direction keys until they have ALL been released.
    // Used on a new stage and when entering/leaving gameplay screens.
    void RequireRelease() noexcept;

private:
    static constexpr float RepeatInterval = 0.12f;

    MovementButtons m_previous;
    CellPosition m_activeDirection = { 0, 0 };
    float m_elapsedTime = 0.0f;
    bool m_waitingForRelease = true;
};
