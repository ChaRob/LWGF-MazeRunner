#pragma once

#include "../maze/CellPosition.h"
#include "../GameTuning.h"

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

// Traversable directions from the player's current logical cell.
struct MovementAvailability
{
    bool up = true;
    bool down = true;
    bool left = true;
    bool right = true;
};

// Converts held direction keys to discrete grid steps without changing Player physics.
class HeldMovementInput
{
public:
    // A held turn takes priority at the first open junction; blocked turns do not stop
    // an otherwise traversable current direction. All steps share one cooldown.
    CellPosition Update(float _deltaTime, const MovementButtons& _buttons,
        const MovementAvailability& _available = {});

    // Ignore all held direction keys until they have ALL been released.
    // Used on a new stage and when entering/leaving gameplay screens.
    void RequireRelease() noexcept;

private:
    static constexpr float RepeatInterval = GameTuning::Movement::RepeatIntervalSeconds;

    MovementButtons m_previous;
    CellPosition m_preferredDirection = { 0, 0 };
    CellPosition m_activeDirection = { 0, 0 };
    // Time since the last emitted move; starts ready for a first press.
    float m_elapsedTime = RepeatInterval;
    bool m_waitingForRelease = true;
};
