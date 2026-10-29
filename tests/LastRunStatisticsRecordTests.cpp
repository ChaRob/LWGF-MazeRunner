#include "../src/game/LastRunStatisticsRecord.h"
#include "../src/game/GameSession.h"
#include "../src/game/GameTuning.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace
{
    void Require(bool _ok, const char* _message)
    {
        if (!_ok) throw std::runtime_error(_message);
    }

    void RequireSame(const RunResultSnapshot& _a, const RunResultSnapshot& _b)
    {
        Require(_a.completedStages == _b.completedStages
            && _a.reachedStage == _b.reachedStage
            && std::fabs(_a.playingSeconds - _b.playingSeconds) < 0.00001
            && _a.successfulMoves == _b.successfulMoves,
            "Saved/reloaded stage progress, time, and movement must match");
        for (std::size_t i = 0; i < _a.items.size(); ++i)
        {
            const auto a = _a.items[i], b = _b.items[i];
            Require(a.spawned == b.spawned && a.pickedUp == b.pickedUp
                && a.used == b.used, "All four item counters must be preserved");
        }
    }

    void TestLastRunPersistence()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto folder = std::filesystem::temp_directory_path()
            / ("MazeRunnerLastRunTest_" + std::to_string(nonce));
        const auto path = folder / "last_run.dat";

        try
        {
            GameSession first(7u);
            const RunResultSnapshot snapshot = CaptureRunResult(first);
            {
                LastRunStatisticsRecord store(path);
                Require(!store.GetLastRun() && !store.HasStorageError(),
                    "No last run file should be an empty, valid initial state");
                Require(!store.SaveRun(first) && !store.GetLastRun(),
                    "An unfinished game must not overwrite the last-run record");
                first.Update(3.5f);
                first.Update(GameTuning::Time::InitialSeconds);
                Require(first.GetStatus() == GameStatus::GameOver,
                    "First game should end before persistence");
                Require(store.SaveRun(first) && !store.HasStorageError(),
                    "Finished game should save successfully");
                Require(store.GetLastRun().has_value(),
                    "Finished run should be available from menu immediately");
                const auto& stored = *store.GetLastRun();
                Require(stored.completedStages == snapshot.completedStages
                    && stored.reachedStage == snapshot.reachedStage
                    && stored.playingSeconds == GameTuning::Time::InitialSeconds,
                    "Saved statistics must reflect Game Over snapshot");
                for (std::size_t i = 0; i < stored.items.size(); ++i)
                {
                    Require(stored.items[i].spawned == snapshot.items[i].spawned,
                        "Last run must preserve item spawn counts");
                }
            }
            {
                LastRunStatisticsRecord reloaded(path);
                Require(reloaded.GetLastRun() && !reloaded.HasStorageError(),
                    "Main menu must load last run after application restart");
                RequireSame(*reloaded.GetLastRun(), CaptureRunResult(first));

                GameSession second(19u);
                second.Update(GameTuning::Time::InitialSeconds);
                Require(reloaded.SaveRun(second), "Newly completed run should replace old");
                RequireSame(*reloaded.GetLastRun(), CaptureRunResult(second));
            }
            {
                LastRunStatisticsRecord reloaded(path);
                GameSession second(19u);
                second.Update(GameTuning::Time::InitialSeconds);
                RequireSame(*reloaded.GetLastRun(), CaptureRunResult(second));
                Require(!reloaded.HasStorageError(),
                    "Replaced last run should survive application restart");
            }
            {
                std::ofstream corrupt(path, std::ios::trunc);
                corrupt << "MAZERUNNER_LAST_RUN_V1\n0\n1\n0\n0\n-1 1 0\n";
            }
            {
                LastRunStatisticsRecord damaged(path);
                Require(!damaged.GetLastRun() && damaged.HasStorageError(),
                    "Malformed counters must not load as a valid run");
                GameSession recovery(29u);
                recovery.Update(GameTuning::Time::InitialSeconds);
                Require(damaged.SaveRun(recovery) && !damaged.HasStorageError(),
                    "A future completed run should recover corrupted storage");
            }
            const auto blockingFile = folder / "blocker";
            {
                std::ofstream blocker(blockingFile);
                blocker << "not a directory";
            }
            LastRunStatisticsRecord failing(blockingFile / "last_run.dat");
            GameSession ended(17u);
            ended.Update(GameTuning::Time::InitialSeconds);
            Require(!failing.SaveRun(ended) && failing.HasStorageError()
                && failing.GetLastRun().has_value(),
                "Filesystem failures retain in-memory result and report an error");
            RequireSame(*failing.GetLastRun(), CaptureRunResult(ended));
        }
        catch (...)
        {
            std::error_code error;
            std::filesystem::remove_all(folder, error);
            throw;
        }
        std::error_code error;
        std::filesystem::remove_all(folder, error);
    }
}

int main()
{
    try
    {
        TestLastRunPersistence();
        std::cout << "Last-run persistence tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Last-run persistence tests failed: " << error.what() << '\n';
        return 1;
    }
}
