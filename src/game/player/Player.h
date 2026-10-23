#pragma once

#include "../maze/CellPosition.h"

class Maze;

class Player
{
public:
    explicit Player(const CellPosition& _startPosition);

    const CellPosition& GetPosition() const noexcept;
    void ResetPosition(const CellPosition& _position) noexcept;

    bool TryMove(
        const Maze& _maze,
        const CellPosition& _direction);

private:
    CellPosition m_position;
};
