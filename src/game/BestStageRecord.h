#pragma once

#include <cstdint>
#include <filesystem>

// Local, unverified personal record. This is intentionally separate from
// a future authenticated account and must not be trusted as an online score.
class BestStageRecord
{
public:
    explicit BestStageRecord(std::filesystem::path _filePath = DefaultFilePath());

    // Returns true when the in-memory best increased, even if disk I/O failed.
    // Call HasStorageError() to detect unsuccessful persistence.
    bool TryUpdate(std::uint64_t _completedStages);

    std::uint64_t GetBest() const noexcept;
    bool HasStorageError() const noexcept;
    const std::filesystem::path& GetFilePath() const noexcept;

    static std::filesystem::path DefaultFilePath();

private:
    void Load();
    bool Save() const;

    std::filesystem::path m_filePath;
    std::uint64_t m_best = 0;
    bool m_storageError = false;
};
