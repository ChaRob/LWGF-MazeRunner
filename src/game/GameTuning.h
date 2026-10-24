#pragma once

#include <cstdint>

// Developer-only compile-time tuning. Change values here, then rebuild MazeRunner.
// This file is compiled into the executable; no player-editable config is shipped.
namespace GameTuning
{
    namespace Movement
    {
        // Minimum time between grid moves, including changes of direction.
        inline constexpr float RepeatIntervalSeconds = 0.14f;
    }

    namespace Time
    {
        inline constexpr float InitialSeconds = 30.0f;
        inline constexpr float MaximumSeconds = 30.0f;
        inline constexpr float ClearBonusSecondsPerCell = 0.20f;
    }

    namespace TimeOrb
    {
        inline constexpr std::uint64_t EarlyStageEnd = 30;
        inline constexpr std::uint64_t MiddleStageEnd = 40;
        inline constexpr std::uint64_t LateStageEnd = 50;
        inline constexpr double EarlySpawnChance = 0.10;
        inline constexpr double MiddleSpawnChance = 0.15;
        inline constexpr double LateSpawnChance = 0.25;
        inline constexpr double EndgameSpawnChance = 0.40;
        inline constexpr float RecoverySeconds = 5.0f;
        inline constexpr float PickupNoticeSeconds = 1.0f;

        constexpr double GetSpawnChance(std::uint64_t _stageNumber) noexcept
        {
            return _stageNumber <= EarlyStageEnd ? EarlySpawnChance
                : _stageNumber <= MiddleStageEnd ? MiddleSpawnChance
                : _stageNumber <= LateStageEnd ? LateSpawnChance
                : EndgameSpawnChance;
        }
    }

    namespace Inventory
    {
        inline constexpr int RecallMaxCount = 2;
        inline constexpr int WallBreakMaxCount = 2;
        inline constexpr int TeleportMaxCount = 1;
    }

    namespace Recall
    {
        inline constexpr std::uint64_t EarlyStageEnd = 30;
        inline constexpr std::uint64_t MiddleStageEnd = 40;
        inline constexpr double EarlySpawnChance = 0.05;
        inline constexpr double MiddleSpawnChance = 0.075;
        inline constexpr double LateSpawnChance = 0.10;
        inline constexpr double EndgameSpawnChance = 0.125;
        inline constexpr std::uint64_t LateStageEnd = 50;

        constexpr double GetSpawnChance(std::uint64_t _stageNumber) noexcept
        {
            return _stageNumber <= EarlyStageEnd ? EarlySpawnChance
                : _stageNumber <= MiddleStageEnd ? MiddleSpawnChance
                : _stageNumber <= LateStageEnd ? LateSpawnChance
                : EndgameSpawnChance;
        }
    }

    namespace WallBreak
    {
        // First pass values for playtesting. Adjust here without changing
        // item placement or inventory logic.
        inline constexpr std::uint64_t EarlyStageEnd = 30;
        inline constexpr std::uint64_t MiddleStageEnd = 40;
        inline constexpr double EarlySpawnChance = 0.05;
        inline constexpr double MiddleSpawnChance = 0.075;
        inline constexpr double LateSpawnChance = 0.10;
        inline constexpr double EndgameSpawnChance = 0.125;
        inline constexpr std::uint64_t LateStageEnd = 50;

        constexpr double GetSpawnChance(std::uint64_t _stageNumber) noexcept
        {
            return _stageNumber <= EarlyStageEnd ? EarlySpawnChance
                : _stageNumber <= MiddleStageEnd ? MiddleSpawnChance
                : _stageNumber <= LateStageEnd ? LateSpawnChance
                : EndgameSpawnChance;
        }
    }

    namespace Teleport
    {
        inline constexpr std::uint64_t EarlyStageEnd = 30;
        inline constexpr std::uint64_t MiddleStageEnd = 40;
        inline constexpr std::uint64_t LateStageEnd = 50;
        // Teleport is more powerful than a wall break. Start rare.
        inline constexpr double EarlySpawnChance = 0.02;
        inline constexpr double MiddleSpawnChance = 0.03;
        inline constexpr double LateSpawnChance = 0.05;
        inline constexpr double EndgameSpawnChance = 0.075;
        inline constexpr int MaximumManhattanDistanceCells = 5;

        constexpr double GetSpawnChance(std::uint64_t _stageNumber) noexcept
        {
            return _stageNumber <= EarlyStageEnd ? EarlySpawnChance
                : _stageNumber <= MiddleStageEnd ? MiddleSpawnChance
                : _stageNumber <= LateStageEnd ? LateSpawnChance
                : EndgameSpawnChance;
        }
    }

    namespace Maze
    {
        inline constexpr int InitialCellSize = 5;
        inline constexpr int MaximumCellSize = 25;
        inline constexpr int CellGrowthPerTier = 2;
        inline constexpr std::uint64_t StagesPerTier = 10;
        inline constexpr float GoalMinimumDistanceRatio = 0.9f;
    }

    namespace Transition
    {
        inline constexpr float FadeSeconds = 0.35f;
        inline constexpr int CountdownSeconds = 3;
        inline constexpr float GoSeconds = 0.4f;
        inline constexpr float ClearNoticeSeconds = 1.0f;
    }
}

static_assert(GameTuning::Transition::ClearNoticeSeconds > 0.0f,
    "Stage clear notice duration must be positive.");
static_assert(GameTuning::Movement::RepeatIntervalSeconds > 0.0f,
    "Movement interval must be positive.");
static_assert(GameTuning::Time::InitialSeconds > 0.0f
    && GameTuning::Time::MaximumSeconds >= GameTuning::Time::InitialSeconds,
    "The maximum time must be at least the positive starting time.");
static_assert(GameTuning::Time::ClearBonusSecondsPerCell >= 0.0f,
    "Clear bonus cannot be negative.");
static_assert(GameTuning::Maze::InitialCellSize >= 2
    && GameTuning::Maze::MaximumCellSize >= GameTuning::Maze::InitialCellSize
    && GameTuning::Maze::CellGrowthPerTier > 0
    && GameTuning::Maze::StagesPerTier > 0,
    "Maze stage-size settings must be positive and ordered.");
static_assert((GameTuning::Maze::MaximumCellSize - GameTuning::Maze::InitialCellSize)
    % GameTuning::Maze::CellGrowthPerTier == 0,
    "Maximum maze size must be reachable in whole growth steps.");
static_assert(GameTuning::Maze::GoalMinimumDistanceRatio > 0.0f
    && GameTuning::Maze::GoalMinimumDistanceRatio <= 1.0f,
    "Goal distance ratio must be in (0, 1].");
static_assert(GameTuning::Transition::FadeSeconds > 0.0f
    && GameTuning::Transition::GoSeconds > 0.0f,
    "Fade and GO durations must be positive.");
static_assert(GameTuning::Transition::CountdownSeconds >= 1
    && GameTuning::Transition::CountdownSeconds <= 9,
    "Countdown must last between one and nine whole seconds.");

static_assert(GameTuning::TimeOrb::EarlyStageEnd > 0
    && GameTuning::TimeOrb::MiddleStageEnd > GameTuning::TimeOrb::EarlyStageEnd
    && GameTuning::TimeOrb::LateStageEnd > GameTuning::TimeOrb::MiddleStageEnd,
    "Time Orb stage tiers must be strictly increasing.");
static_assert(GameTuning::TimeOrb::EarlySpawnChance >= 0.0
    && GameTuning::TimeOrb::EarlySpawnChance <= 1.0
    && GameTuning::TimeOrb::MiddleSpawnChance >= 0.0
    && GameTuning::TimeOrb::MiddleSpawnChance <= 1.0
    && GameTuning::TimeOrb::LateSpawnChance >= 0.0
    && GameTuning::TimeOrb::LateSpawnChance <= 1.0
    && GameTuning::TimeOrb::EndgameSpawnChance >= 0.0
    && GameTuning::TimeOrb::EndgameSpawnChance <= 1.0,
    "Time Orb spawn probabilities must be between zero and one.");
static_assert(GameTuning::TimeOrb::RecoverySeconds > 0.0f
    && GameTuning::TimeOrb::PickupNoticeSeconds > 0.0f,
    "Time Orb recovery and pickup notice durations must be positive.");

static_assert(GameTuning::Inventory::RecallMaxCount > 0
    && GameTuning::Inventory::WallBreakMaxCount > 0
    && GameTuning::Inventory::TeleportMaxCount > 0,
    "Inventory item capacity must be positive.");
static_assert(GameTuning::Recall::EarlyStageEnd > 0
    && GameTuning::Recall::MiddleStageEnd > GameTuning::Recall::EarlyStageEnd,
    "Recall tiers must be increasing.");
static_assert(GameTuning::Recall::EarlySpawnChance >= 0.0
    && GameTuning::Recall::EarlySpawnChance <= 1.0
    && GameTuning::Recall::MiddleSpawnChance >= 0.0
    && GameTuning::Recall::MiddleSpawnChance <= 1.0
    && GameTuning::Recall::LateSpawnChance >= 0.0
    && GameTuning::Recall::LateSpawnChance <= 1.0,
    "Recall spawn probabilities must be between zero and one.");

static_assert(GameTuning::WallBreak::EarlyStageEnd > 0
    && GameTuning::WallBreak::MiddleStageEnd > GameTuning::WallBreak::EarlyStageEnd,
    "Wall Break stage tiers must be strictly increasing.");
static_assert(GameTuning::WallBreak::EarlySpawnChance >= 0.0
    && GameTuning::WallBreak::EarlySpawnChance <= 1.0
    && GameTuning::WallBreak::MiddleSpawnChance >= 0.0
    && GameTuning::WallBreak::MiddleSpawnChance <= 1.0
    && GameTuning::WallBreak::LateSpawnChance >= 0.0
    && GameTuning::WallBreak::LateSpawnChance <= 1.0,
    "Wall Break spawn probabilities must be in [0, 1].");

static_assert(GameTuning::Recall::LateStageEnd > GameTuning::Recall::MiddleStageEnd
    && GameTuning::Recall::EndgameSpawnChance >= 0.0
    && GameTuning::Recall::EndgameSpawnChance <= 1.0,
    "Recall endgame tuning must be valid.");
static_assert(GameTuning::WallBreak::LateStageEnd > GameTuning::WallBreak::MiddleStageEnd
    && GameTuning::WallBreak::EndgameSpawnChance >= 0.0
    && GameTuning::WallBreak::EndgameSpawnChance <= 1.0,
    "Wall Break endgame tuning must be valid.");
static_assert(GameTuning::Teleport::EarlyStageEnd > 0
    && GameTuning::Teleport::MiddleStageEnd > GameTuning::Teleport::EarlyStageEnd
    && GameTuning::Teleport::LateStageEnd > GameTuning::Teleport::MiddleStageEnd,
    "Teleport stage tiers must increase.");
static_assert(GameTuning::Teleport::EarlySpawnChance >= 0.0
    && GameTuning::Teleport::EarlySpawnChance <= 1.0
    && GameTuning::Teleport::MiddleSpawnChance >= 0.0
    && GameTuning::Teleport::MiddleSpawnChance <= 1.0
    && GameTuning::Teleport::LateSpawnChance >= 0.0
    && GameTuning::Teleport::LateSpawnChance <= 1.0
    && GameTuning::Teleport::EndgameSpawnChance >= 0.0
    && GameTuning::Teleport::EndgameSpawnChance <= 1.0,
    "Teleport spawn probabilities must be in [0, 1].");
static_assert(GameTuning::Teleport::MaximumManhattanDistanceCells > 1,
    "Teleport range must be longer than a normal adjacent move.");
