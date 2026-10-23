#include "../src/game/BestStageRecord.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
    void Require(bool _condition, const char* _message)
    {
        if (!_condition)
        {
            throw std::runtime_error(_message);
        }
    }

    void TestLocalBestRecord()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::filesystem::path folder =
            std::filesystem::temp_directory_path()
            / ("MazeRunnerBestRecordTest_" + std::to_string(nonce));
        const std::filesystem::path file = folder / "best_stage.dat";

        try
        {
            {
                BestStageRecord record(file);
                Require(record.GetBest() == 0 && !record.HasStorageError(),
                    "A new player begins with no cleared stages");
                Require(!record.TryUpdate(0), "Zero clears must not create a new record");
                Require(record.TryUpdate(1), "First completed stage sets a new record");
                Require(!record.HasStorageError(), "New record should save successfully");
                Require(!record.TryUpdate(1), "An equal record is not an improvement");
                Require(!record.TryUpdate(0), "A worse run must not replace the record");
                Require(record.TryUpdate(37), "The highest completed stage must replace the best");
                Require(record.GetBest() == 37, "In-memory record must update immediately");
            }
            {
                BestStageRecord reloaded(file);
                Require(reloaded.GetBest() == 37 && !reloaded.HasStorageError(),
                    "Record must persist across a fresh game launch");
                Require(!reloaded.TryUpdate(25), "A worse run must not overwrite disk");
                Require(reloaded.TryUpdate(38), "A new high stage must update persistent storage");
            }
            {
                BestStageRecord reloaded(file);
                Require(reloaded.GetBest() == 38, "Updated record must persist immediately");
            }
            {
                std::ofstream corrupted(file, std::ios::trunc);
                corrupted << "MAZERUNNER_BEST_V1\nnot-a-number\n";
            }
            {
                BestStageRecord corrupted(file);
                Require(corrupted.GetBest() == 0 && corrupted.HasStorageError(),
                    "Invalid record data must be rejected and surfaced as storage error");
                Require(corrupted.TryUpdate(4), "A legitimate new run can repair corrupted data");
                Require(!corrupted.HasStorageError(),
                    "Successful save must clear the previous storage error");
            }
            {
                BestStageRecord repaired(file);
                Require(repaired.GetBest() == 4 && !repaired.HasStorageError(),
                    "Repaired record must reload");
            }

            // A regular file cannot act as a parent directory.
            const std::filesystem::path blocker = folder / "not_a_directory";
            {
                std::ofstream blockerFile(blocker);
                blockerFile << "block";
            }
            BestStageRecord failing(blocker / "best_stage.dat");
            Require(failing.TryUpdate(10), "Progress still updates in memory on disk failure");
            Require(failing.GetBest() == 10 && failing.HasStorageError(),
                "Save failure must be surfaced rather than silently claiming persistence");
        }
        catch (...)
        {
            std::error_code ec;
            std::filesystem::remove_all(folder, ec);
            throw;
        }

        std::error_code ec;
        std::filesystem::remove_all(folder, ec);
    }
}

int main()
{
    try
    {
        TestLocalBestRecord();
        std::cout << "BestStageRecordTests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
