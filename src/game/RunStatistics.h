#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// A single run's counters, not persisted across game restarts.
// Spawned counts stages where an item appeared, even if never picked up.
// Pickup is credited only when the world item is removed successfully.
// Used is credited only on successful active-item use; Time Orb is automatic.
enum class RunItemType : std::size_t
{
    TimeOrb,
    Recall,
    WallBreak,
    Teleport,
    Count
};

struct RunItemCounters
{
    std::uint64_t spawned = 0;
    std::uint64_t pickedUp = 0;
    std::uint64_t used = 0;
};

class RunStatistics
{
public:
    void AddPlayingSeconds(double _seconds) noexcept
    {
        if (_seconds > 0.0)
        {
            m_playingSeconds += _seconds;
        }
    }

    void RecordSuccessfulMove() noexcept { ++m_successfulMoves; }

    void RecordSpawn(RunItemType _type) noexcept
    {
        if (auto* item = Find(_type)) ++item->spawned;
    }

    void RecordPickup(RunItemType _type) noexcept
    {
        if (auto* item = Find(_type)) ++item->pickedUp;
    }

    void RecordUse(RunItemType _type) noexcept
    {
        if (auto* item = Find(_type)) ++item->used;
    }

    double GetPlayingSeconds() const noexcept { return m_playingSeconds; }
    std::uint64_t GetSuccessfulMoves() const noexcept { return m_successfulMoves; }

    RunItemCounters GetItem(RunItemType _type) const noexcept
    {
        const auto index = static_cast<std::size_t>(_type);
        return index < m_items.size() ? m_items[index] : RunItemCounters{};
    }

private:
    RunItemCounters* Find(RunItemType _type) noexcept
    {
        const auto index = static_cast<std::size_t>(_type);
        return index < m_items.size() ? &m_items[index] : nullptr;
    }

    double m_playingSeconds = 0.0;
    std::uint64_t m_successfulMoves = 0;
    std::array<RunItemCounters, static_cast<std::size_t>(RunItemType::Count)> m_items{};
};
