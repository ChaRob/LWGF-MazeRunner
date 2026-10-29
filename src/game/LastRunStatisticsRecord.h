#pragma once

#include "RunStatistics.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>

class GameSession;

struct RunResultSnapshot
{
    std::uint64_t completedStages = 0;
    std::uint64_t reachedStage = 0;
    double playingSeconds = 0.0;
    std::uint64_t successfulMoves = 0;
    std::array<RunItemCounters, static_cast<std::size_t>(RunItemType::Count)> items{};

    RunItemCounters GetItem(RunItemType _type) const noexcept
    {
        const auto index = static_cast<std::size_t>(_type);
        return index < items.size() ? items[index] : RunItemCounters{};
    }
};

RunResultSnapshot CaptureRunResult(const GameSession& _game) noexcept;

// Keeps only the last completed run. A new run does not erase previous results,
// and the save lives beside best_stage.dat across application restarts.
class LastRunStatisticsRecord
{
public:
    explicit LastRunStatisticsRecord(
        std::filesystem::path _filePath = DefaultFilePath());

    // Updates in-memory results even when disk I/O fails.
    bool SaveRun(const GameSession& _game);
    const std::optional<RunResultSnapshot>& GetLastRun() const noexcept;
    bool HasStorageError() const noexcept;
    static std::filesystem::path DefaultFilePath();

private:
    void Load();
    bool Save() const;

    std::filesystem::path m_filePath;
    std::optional<RunResultSnapshot> m_lastRun;
    bool m_storageError = false;
};
