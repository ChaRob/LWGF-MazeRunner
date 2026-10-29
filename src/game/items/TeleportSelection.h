#pragma once

#include "../items/ActiveItemInventory.h"
#include "../stage/Stage.h"

#include <cstdlib>

// Keyboard-only Teleport aiming. The player stays still until confirmation.
// This state is presentation/input selection only: it never consumes items.
class TeleportSelection
{
public:
    bool Begin(const Stage& _stage, const ActiveItemInventory& _inventory) noexcept
    {
        if (m_active || _inventory.GetCount(ActiveItemType::Teleport) <= 0)
        {
            return false;
        }

        m_origin = _stage.GetPlayer().GetPosition();
        m_selected = m_origin;
        m_active = true;
        return true;
    }

    bool IsActive() const noexcept { return m_active; }
    const CellPosition& GetSelectedPosition() const noexcept { return m_selected; }
    bool IsAtOrigin() const noexcept { return m_active && m_selected == m_origin; }

    bool CanSelect(const Stage& _stage, const CellPosition& _position) const noexcept
    {
        // The player position is a cancellation target. All other cells must
        // satisfy the same rules as an actual Teleport, including Goal ban.
        return m_active && (_position == m_origin || _stage.CanTeleportTo(_position));
    }

    bool TryMoveCursor(const Stage& _stage, const CellPosition& _direction) noexcept
    {
        if (!m_active || std::abs(_direction.x) + std::abs(_direction.y) != 1)
        {
            return false;
        }

        const CellPosition next{ m_selected.x + _direction.x,
            m_selected.y + _direction.y };
        if (!CanSelect(_stage, next))
        {
            return false;
        }

        m_selected = next;
        return true;
    }

    void Cancel() noexcept
    {
        m_active = false;
        m_origin = { 0, 0 };
        m_selected = { 0, 0 };
    }

private:
    bool m_active = false;
    CellPosition m_origin{ 0, 0 };
    CellPosition m_selected{ 0, 0 };
};
