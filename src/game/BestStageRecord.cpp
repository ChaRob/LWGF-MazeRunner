#include "BestStageRecord.h"

#include <charconv>
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
    constexpr const char* RecordVersion = "MAZERUNNER_BEST_V1";

    std::filesystem::path UserDataDirectory()
    {
#ifdef _WIN32
        char* localAppData = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&localAppData, &size, "LOCALAPPDATA") == 0
            && localAppData != nullptr && localAppData[0] != '\0')
        {
            const std::filesystem::path result =
                std::filesystem::path(localAppData) / "MazeRunner";
            std::free(localAppData);
            return result;
        }
        std::free(localAppData);
#else
        if (const char* xdgData = std::getenv("XDG_DATA_HOME");
            xdgData != nullptr && xdgData[0] != '\0')
        {
            return std::filesystem::path(xdgData) / "MazeRunner";
        }
        if (const char* home = std::getenv("HOME");
            home != nullptr && home[0] != '\0')
        {
            return std::filesystem::path(home) / ".local" / "share" / "MazeRunner";
        }
#endif
        // Fallback for test environments without a user data directory.
        return std::filesystem::current_path() / "MazeRunnerUserData";
    }
}

BestStageRecord::BestStageRecord(std::filesystem::path _filePath)
    : m_filePath(std::move(_filePath))
{
    Load();
}

std::filesystem::path BestStageRecord::DefaultFilePath()
{
    return UserDataDirectory() / "best_stage.dat";
}

void BestStageRecord::Load()
{
    std::error_code ec;
    if (!std::filesystem::exists(m_filePath, ec))
    {
        m_storageError = static_cast<bool>(ec);
        return;
    }

    std::ifstream file(m_filePath, std::ios::binary);
    std::string version;
    std::string value;
    std::string unexpected;
    if (!file || !std::getline(file, version) || version != RecordVersion
        || !std::getline(file, value) || std::getline(file, unexpected))
    {
        m_storageError = true;
        return;
    }

    std::uint64_t parsed = 0;
    const auto result = std::from_chars(
        value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size())
    {
        m_storageError = true;
        return;
    }

    m_best = parsed;
}

bool BestStageRecord::Save() const
{
    std::error_code ec;
    const std::filesystem::path directory = m_filePath.parent_path();
    if (!directory.empty())
    {
        std::filesystem::create_directories(directory, ec);
        if (ec)
        {
            return false;
        }
    }

    std::filesystem::path temporaryPath = m_filePath;
    temporaryPath += ".tmp";

    {
        std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            return false;
        }
        file << RecordVersion << '\n' << m_best << '\n';
        file.flush();
        if (!file)
        {
            return false;
        }
        file.close();
        if (!file)
        {
            return false;
        }
    }

#ifdef _WIN32
    // std::filesystem::rename cannot replace an existing file on Windows.
    if (!MoveFileExW(temporaryPath.c_str(), m_filePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(temporaryPath, ec);
        return false;
    }
#else
    std::filesystem::rename(temporaryPath, m_filePath, ec);
    if (ec)
    {
        std::filesystem::remove(temporaryPath, ec);
        return false;
    }
#endif
    return true;
}

bool BestStageRecord::TryUpdate(std::uint64_t _completedStages)
{
    if (_completedStages <= m_best)
    {
        return false;
    }

    m_best = _completedStages;
    m_storageError = !Save();
    return true;
}

std::uint64_t BestStageRecord::GetBest() const noexcept
{
    return m_best;
}

bool BestStageRecord::HasStorageError() const noexcept
{
    return m_storageError;
}

const std::filesystem::path& BestStageRecord::GetFilePath() const noexcept
{
    return m_filePath;
}
