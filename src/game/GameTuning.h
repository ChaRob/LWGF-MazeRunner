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

    namespace Score
    {
        inline constexpr std::uint64_t BasePointsPerStage = 100;
        inline constexpr std::uint64_t DistancePointsPerCell = 10;
        // Independent of the time awarded on clearing a stage.
        inline constexpr float SpeedBaselineSecondsPerCell = 0.5f;
        inline constexpr float SpeedPointsPerSecondSaved = 10.0f;
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
static_assert(GameTuning::Score::SpeedBaselineSecondsPerCell > 0.0f
    && GameTuning::Score::SpeedPointsPerSecondSaved >= 0.0f,
    "Speed scoring settings must be valid.");
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
