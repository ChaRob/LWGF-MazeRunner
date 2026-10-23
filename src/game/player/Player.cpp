#include "Player.h"

#include "../maze/Maze.h"

Player::Player(const CellPosition& _startPosition)
    : m_position(_startPosition)
{}

const CellPosition& Player::GetPosition() const noexcept
{
    return m_position;
}

void Player::ResetPosition(const CellPosition& _position) noexcept
{
    m_position = _position;
}

bool Player::TryMove(
    const Maze& _maze,
    const CellPosition& _direction)
{
    const CellPosition nextPosition =
    {
        m_position.x + _direction.x,
        m_position.y + _direction.y
    };

    if (!_maze.CanMove(m_position, nextPosition))
    {
        return false;
    }

    m_position = nextPosition;
    return true;
}
