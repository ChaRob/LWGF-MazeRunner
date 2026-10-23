#pragma once

#include "../GameTuning.h"

#include <array>
#include <cstddef>

enum class ActiveItemType : std::size_t
{
    Recall,
    WallBreak,
    Teleport,
    Count
};

// A session-owned inventory; counts carry between stages, not between runs.
// WallBreak and Teleport are reserved for future implementations.
class ActiveItemInventory
{
public:
    static constexpr int GetCapacity(ActiveItemType _type) noexcept
    {
        switch (_type)
        {
        case ActiveItemType::Recall:
            return GameTuning::Inventory::RecallMaxCount;
        case ActiveItemType::WallBreak:
            return GameTuning::Inventory::WallBreakMaxCount;
        case ActiveItemType::Teleport:
            return GameTuning::Inventory::TeleportMaxCount;
        default:
            return 0;
        }
    }

    int GetCount(ActiveItemType _type) const noexcept
    {
        const std::size_t index = static_cast<std::size_t>(_type);
        return index < m_counts.size() ? m_counts[index] : 0;
    }

    bool TryAdd(ActiveItemType _type) noexcept
    {
        const int cap = GetCapacity(_type);
        if (cap <= 0 || GetCount(_type) >= cap)
        {
            return false;
        }

        ++m_counts[static_cast<std::size_t>(_type)];
        return true;
    }

    bool TryConsume(ActiveItemType _type) noexcept
    {
        if (GetCount(_type) <= 0)
        {
            return false;
        }

        --m_counts[static_cast<std::size_t>(_type)];
        return true;
    }

private:
    std::array<int, static_cast<std::size_t>(ActiveItemType::Count)> m_counts{};
};
