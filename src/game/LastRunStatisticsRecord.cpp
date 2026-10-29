#include "LastRunStatisticsRecord.h"

#include "BestStageRecord.h"
#include "GameSession.h"

#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
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
    constexpr const char* FormatVersion = "MAZERUNNER_LAST_RUN_V1";

    bool ParseUnsigned(const std::string& _line, std::uint64_t& _value)
    {
        if (_line.empty()) return false;
        const auto result = std::from_chars(
            _line.data(), _line.data() + _line.size(), _value);
        return result.ec == std::errc{}
            && result.ptr == _line.data() + _line.size();
    }

    bool ParseSeconds(const std::string& _line, double& _seconds)
    {
        std::istringstream input(_line);
        input.imbue(std::locale::classic());
        if (!(input >> _seconds) || !std::isfinite(_seconds) || _seconds < 0.0)
            return false;
        input >> std::ws;
        return input.eof();
    }

    bool ParseCounters(const std::string& _line, RunItemCounters& _value)
    {
        std::istringstream input(_line);
        std::string spawn, pickup, used, extra;
        if (!(input >> spawn >> pickup >> used) || input >> extra)
            return false;
        return ParseUnsigned(spawn, _value.spawned)
            && ParseUnsigned(pickup, _value.pickedUp)
            && ParseUnsigned(used, _value.used)
            && _value.pickedUp <= _value.spawned
            && _value.used <= _value.pickedUp;
    }
}

RunResultSnapshot CaptureRunResult(const GameSession& _game) noexcept
{
    RunResultSnapshot result{};
    result.completedStages = _game.GetCompletedStages();
    result.reachedStage = _game.GetStage().GetNumber();
    result.playingSeconds = _game.GetStatistics().GetPlayingSeconds();
    result.successfulMoves = _game.GetStatistics().GetSuccessfulMoves();

    for (std::size_t i = 0; i < result.items.size(); ++i)
    {
        result.items[i] = _game.GetStatistics().GetItem(
            static_cast<RunItemType>(i));
    }
    return result;
}

LastRunStatisticsRecord::LastRunStatisticsRecord(std::filesystem::path _path)
    : m_filePath(std::move(_path))
{
    Load();
}

std::filesystem::path LastRunStatisticsRecord::DefaultFilePath()
{
    return BestStageRecord::DefaultFilePath().parent_path() / "last_run.dat";
}

void LastRunStatisticsRecord::Load()
{
    std::error_code error;
    if (!std::filesystem::exists(m_filePath, error))
    {
        m_storageError = static_cast<bool>(error);
        return;
    }

    std::ifstream file(m_filePath, std::ios::binary);
    std::string version, line;
    RunResultSnapshot parsed;
    if (!file || !std::getline(file, version) || version != FormatVersion
        || !std::getline(file, line)
        || !ParseUnsigned(line, parsed.completedStages)
        || !std::getline(file, line)
        || !ParseUnsigned(line, parsed.reachedStage)
        || !std::getline(file, line)
        || !ParseSeconds(line, parsed.playingSeconds)
        || !std::getline(file, line)
        || !ParseUnsigned(line, parsed.successfulMoves)
        || parsed.reachedStage == 0
        || parsed.reachedStage < parsed.completedStages)
    {
        m_storageError = true;
        return;
    }

    for (RunItemCounters& counters : parsed.items)
    {
        if (!std::getline(file, line) || !ParseCounters(line, counters))
        {
            m_storageError = true;
            return;
        }
    }
    if (std::getline(file, line))
    {
        m_storageError = true; // Reject unknown trailing data.
        return;
    }
    m_lastRun = parsed;
}

bool LastRunStatisticsRecord::Save() const
{
    if (!m_lastRun) return false;

    std::error_code error;
    const std::filesystem::path folder = m_filePath.parent_path();
    if (!folder.empty())
    {
        std::filesystem::create_directories(folder, error);
        if (error) return false;
    }

    std::filesystem::path temporary = m_filePath;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        const RunResultSnapshot& run = *m_lastRun;
        file.imbue(std::locale::classic());
        file << FormatVersion << '\n'
             << run.completedStages << '\n'
             << run.reachedStage << '\n'
             << std::setprecision(17) << run.playingSeconds << '\n'
             << run.successfulMoves << '\n';
        for (const RunItemCounters& item : run.items)
        {
            file << item.spawned << ' ' << item.pickedUp << ' '
                 << item.used << '\n';
        }
        file.flush();
        if (!file) return false;
        file.close();
        if (!file) return false;
    }

#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), m_filePath.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        std::filesystem::remove(temporary, error);
        return false;
    }
#else
    std::filesystem::rename(temporary, m_filePath, error);
    if (error)
    {
        std::filesystem::remove(temporary, error);
        return false;
    }
#endif
    return true;
}

bool LastRunStatisticsRecord::SaveRun(const GameSession& _game)
{
    if (_game.GetStatus() != GameStatus::GameOver)
        return false;

    m_lastRun = CaptureRunResult(_game);
    m_storageError = !Save();
    return !m_storageError;
}

const std::optional<RunResultSnapshot>& LastRunStatisticsRecord::GetLastRun() const noexcept
{
    return m_lastRun;
}

bool LastRunStatisticsRecord::HasStorageError() const noexcept
{
    return m_storageError;
}
