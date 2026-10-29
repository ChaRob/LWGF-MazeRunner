#include <framework/Application.h>
#include <framework/Font.h>
#include <framework/Renderer2D.h>

#include "game/GameFlow.h"
#include "game/BestStageRecord.h"
#include "game/LastRunStatisticsRecord.h"
#include "game/GameTuning.h"
#include "game/StageClearNotice.h"
#include "game/player/HeldMovementInput.h"
#include "game/player/PlayerMovementVisual.h"
#include "game/items/TeleportSelection.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#ifdef DrawText
#undef DrawText
#endif
#endif

namespace
{
    struct ButtonBounds
    {
        float x;
        float y;
        float width;
        float height;

        bool Contains(double _mouseX, double _mouseY) const noexcept
        {
            return _mouseX >= x && _mouseX <= x + width
                && _mouseY >= y && _mouseY <= y + height;
        }
    };

    constexpr Color TextColor = { 0.96f, 0.97f, 1.0f, 1.0f };
    constexpr Color HighlightColor = { 1.0f, 0.85f, 0.3f, 1.0f };
    constexpr Color BackgroundColor = { 0.055f, 0.075f, 0.11f, 1.0f };
    constexpr Color SubtleTextColor = { 0.65f, 0.70f, 0.79f, 1.0f };
    constexpr Color ButtonColor = { 0.16f, 0.22f, 0.31f, 1.0f };
    constexpr Color HoverColor = { 0.26f, 0.36f, 0.49f, 1.0f };
    constexpr Color WallColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    constexpr Color OuterWallColor = { 0.42f, 0.61f, 0.83f, 1.0f };
    constexpr Color StartColor = { 0.2f, 0.8f, 0.2f, 1.0f };
    constexpr Color GoalColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    constexpr Color PlayerColor = { 0.2f, 0.5f, 1.0f, 1.0f };
    constexpr Color TimeOrbColor = { 0.12f, 0.91f, 0.90f, 1.0f };
    constexpr Color TimeOrbCoreColor = { 0.96f, 1.0f, 1.0f, 1.0f };
    constexpr Color RecallColor = { 0.72f, 0.39f, 0.96f, 1.0f };
    constexpr Color RecallCoreColor = { 1.0f, 0.90f, 1.0f, 1.0f };
    constexpr Color WallBreakColor = { 1.0f, 0.57f, 0.19f, 1.0f };
    constexpr Color WallBreakCoreColor = { 1.0f, 0.91f, 0.62f, 1.0f };
    constexpr Color TeleportColor = { 0.97f, 0.34f, 0.57f, 1.0f };
    constexpr Color TeleportCoreColor = { 1.0f, 0.93f, 0.94f, 1.0f };
    constexpr Color TimerBackground = { 0.25f, 0.25f, 0.25f, 1.0f };
    constexpr Color TimerForeground = { 0.25f, 0.75f, 0.3f, 1.0f };

    std::filesystem::path GetExecutableDirectory(const char* _argv0)
    {
#ifdef _WIN32
        std::wstring executablePath(32768, L'\0');
        const DWORD length = GetModuleFileNameW(nullptr, executablePath.data(),
            static_cast<DWORD>(executablePath.size()));
        if (length == 0 || length >= executablePath.size())
        {
            throw std::runtime_error("Cannot resolve MazeRunner executable directory");
        }
        executablePath.resize(length);
        return std::filesystem::path(executablePath).parent_path();
#else
        return std::filesystem::absolute(_argv0 ? _argv0 : ".").parent_path();
#endif
    }

    ButtonBounds UpperButton(int _width, int _height)
    {
        return { _width * 0.5f - 145.0f, _height * 0.5f - 20.0f, 290.0f, 58.0f };
    }

    ButtonBounds LowerButton(int _width, int _height)
    {
        return { _width * 0.5f - 145.0f, _height * 0.5f - 100.0f, 290.0f, 58.0f };
    }

    ButtonBounds MainMenuExitButton(int _width, int _height)
    {
        return { _width * 0.5f - 145.0f, _height * 0.5f - 180.0f, 290.0f, 58.0f };
    }

    ButtonBounds ResultButton(int _width, int _height)
    {
        // Leave space for the per-run statistics panel above the menu button.
        return { _width * 0.5f - 145.0f, _height * 0.5f - 283.0f, 290.0f, 58.0f };
    }

    bool IsClicked(const Application& _app, const ButtonBounds& _bounds)
    {
        return _app.IsMouseButtonPressed(MouseButton::Left)
            && _bounds.Contains(_app.GetMouseX(), _app.GetMouseY());
    }

    void DrawCenteredText(Renderer2D& _renderer, const Font& _font,
        std::string_view _text, float _centerX, float _baselineY,
        float _height, const Color& _color)
    {
        const float textWidth = _renderer.MeasureTextWidth(_font, _text, _height);
        _renderer.DrawText(_font, _text, _centerX - textWidth * 0.5f,
            _baselineY, _height, _color);
    }

    void DrawButton(Renderer2D& _renderer, const Font& _font,
        std::string_view _text, const ButtonBounds& _bounds, bool _hovered)
    {
        _renderer.DrawRect(_bounds.x, _bounds.y, _bounds.width, _bounds.height,
            _hovered ? HoverColor : ButtonColor);

        const float fontHeight = 28.0f;
        DrawCenteredText(_renderer, _font, _text,
            _bounds.x + _bounds.width * 0.5f,
            _bounds.y + (_bounds.height - fontHeight) * 0.5f + 5.0f,
            fontHeight, TextColor);
    }

    void DrawMainMenu(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const BestStageRecord& _record)
    {
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;
        DrawCenteredText(_renderer, _font, "MAZE RUNNER",
            centerX, centerY + 136.0f, 64.0f, HighlightColor);
        DrawCenteredText(_renderer, _font, "ESCAPE THE MAZE BEFORE TIME RUNS OUT",
            centerX, centerY + 88.0f, 21.0f, SubtleTextColor);

        const ButtonBounds start = UpperButton(_app.GetWidth(), _app.GetHeight());
        const ButtonBounds statistics = LowerButton(_app.GetWidth(), _app.GetHeight());
        const ButtonBounds exit = MainMenuExitButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, "START", start,
            start.Contains(_app.GetMouseX(), _app.GetMouseY()));
        DrawButton(_renderer, _font, "STATISTICS", statistics,
            statistics.Contains(_app.GetMouseX(), _app.GetMouseY()));
        DrawButton(_renderer, _font, "EXIT", exit,
            exit.Contains(_app.GetMouseX(), _app.GetMouseY()));

        char bestText[96] = {};
        std::snprintf(bestText, sizeof(bestText), "PERSONAL BEST  %llu CLEARED",
            static_cast<unsigned long long>(_record.GetBest()));
        DrawCenteredText(_renderer, _font, bestText,
            centerX, centerY - 251.0f, 24.0f, SubtleTextColor);
        if (_record.HasStorageError())
        {
            DrawCenteredText(_renderer, _font, "RECORD SAVE ERROR",
                centerX, centerY - 282.0f, 19.0f, HighlightColor);
        }
    }

    void DrawPausedMenu(Renderer2D& _renderer, const Font& _font,
        const Application& _app)
    {
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;
        // Intentionally do not render Maze, Player, Start or Goal while paused.
        DrawCenteredText(_renderer, _font, "PAUSED",
            centerX, centerY + 136.0f, 64.0f, HighlightColor);
        DrawCenteredText(_renderer, _font, "ESC TO RESUME",
            centerX, centerY + 88.0f, 22.0f, SubtleTextColor);

        const ButtonBounds resume = UpperButton(_app.GetWidth(), _app.GetHeight());
        const ButtonBounds menu = LowerButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, "CONTINUE", resume,
            resume.Contains(_app.GetMouseX(), _app.GetMouseY()));
        DrawButton(_renderer, _font, "MAIN MENU", menu,
            menu.Contains(_app.GetMouseX(), _app.GetMouseY()));
    }

    void DrawRunDetails(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const RunResultSnapshot& _run,
        const BestStageRecord& _record, bool _gameOver, bool _statsStorageError)
    {
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;

        DrawCenteredText(_renderer, _font, _gameOver ? "GAME OVER" : "LAST RUN",
            centerX, centerY + 258.0f, 57.0f, HighlightColor);

        char clearedText[96] = {};
        std::snprintf(clearedText, sizeof(clearedText), "STAGES CLEARED  %llu",
            static_cast<unsigned long long>(_run.completedStages));
        DrawCenteredText(_renderer, _font, clearedText,
            centerX, centerY + 201.0f, 26.0f, TextColor);

        char reachedText[96] = {};
        std::snprintf(reachedText, sizeof(reachedText), "STAGE REACHED   %llu",
            static_cast<unsigned long long>(_run.reachedStage));
        DrawCenteredText(_renderer, _font, reachedText,
            centerX, centerY + 163.0f, 22.0f, SubtleTextColor);

        char bestText[96] = {};
        std::snprintf(bestText, sizeof(bestText), "PERSONAL BEST   %llu",
            static_cast<unsigned long long>(_record.GetBest()));
        DrawCenteredText(_renderer, _font, bestText,
            centerX, centerY + 126.0f, 25.0f, HighlightColor);

        // Seconds are rounded down for display; only the actually active
        // Playing state contributes to this duration (not pause or countdown).
        const std::uint64_t elapsed =
            static_cast<std::uint64_t>(_run.playingSeconds);
        char summary[160] = {};
        std::snprintf(summary, sizeof(summary),
            "PLAY TIME  %02llu:%02llu     MOVES  %llu",
            static_cast<unsigned long long>(elapsed / 60),
            static_cast<unsigned long long>(elapsed % 60),
            static_cast<unsigned long long>(_run.successfulMoves));
        DrawCenteredText(_renderer, _font, summary,
            centerX, centerY + 77.0f, 22.0f, TextColor);

        constexpr float panelWidth = 780.0f;
        constexpr float panelHeight = 196.0f;
        const float panelX = centerX - panelWidth * 0.5f;
        const float panelY = centerY - 137.0f;
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.10f, 0.14f, 0.20f, 1.0f });
        _renderer.DrawRect(panelX, panelY + panelHeight - 3.0f,
            panelWidth, 3.0f, HighlightColor);

        const float labelX = panelX + 31.0f;
        const float spawnedX = panelX + 300.0f;
        const float pickedUpX = panelX + 464.0f;
        const float usedX = panelX + 630.0f;
        const float headerY = centerY + 27.0f;
        _renderer.DrawText(_font, "ITEM", labelX, headerY, 19.0f, SubtleTextColor);
        _renderer.DrawText(_font, "SPAWN", spawnedX, headerY, 19.0f, SubtleTextColor);
        _renderer.DrawText(_font, "PICKUP", pickedUpX, headerY, 19.0f, SubtleTextColor);
        _renderer.DrawText(_font, "USED", usedX, headerY, 19.0f, SubtleTextColor);

        struct ItemResultRow
        {
            const char* name;
            RunItemType type;
            Color color;
            bool isConsumable;
        };
        const ItemResultRow rows[] =
        {
            { "TIME ORB", RunItemType::TimeOrb, TimeOrbColor, false },
            { "RECALL", RunItemType::Recall, RecallColor, true },
            { "WALL BREAK", RunItemType::WallBreak, WallBreakColor, true },
            { "TELEPORT", RunItemType::Teleport, TeleportColor, true }
        };
        for (int i = 0; i < 4; ++i)
        {
            const ItemResultRow& row = rows[i];
            const RunItemCounters totals = _run.GetItem(row.type);
            const float y = centerY - 9.0f - 35.0f * static_cast<float>(i);
            _renderer.DrawText(_font, row.name, labelX, y, 21.0f, row.color);

            char count[40] = {};
            std::snprintf(count, sizeof(count), "%llu",
                static_cast<unsigned long long>(totals.spawned));
            _renderer.DrawText(_font, count, spawnedX, y, 21.0f, TextColor);

            std::snprintf(count, sizeof(count), "%llu",
                static_cast<unsigned long long>(totals.pickedUp));
            _renderer.DrawText(_font, count, pickedUpX, y, 21.0f, TextColor);

            if (row.isConsumable)
            {
                std::snprintf(count, sizeof(count), "%llu",
                    static_cast<unsigned long long>(totals.used));
            }
            else
            {
                std::snprintf(count, sizeof(count), "-");
            }
            _renderer.DrawText(_font, count, usedX, y, 21.0f, TextColor);
        }

        if (_record.HasStorageError())
        {
            DrawCenteredText(_renderer, _font, "RECORD SAVE ERROR",
                centerX, centerY - 170.0f, 19.0f, HighlightColor);
        }

        if (_statsStorageError)
        {
            DrawCenteredText(_renderer, _font, "STATS SAVE ERROR",
                centerX, centerY - 193.0f, 18.0f, HighlightColor);
        }

        const ButtonBounds menu = ResultButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, _gameOver ? "MAIN MENU" : "BACK", menu,
            menu.Contains(_app.GetMouseX(), _app.GetMouseY()));
    }

    void DrawResult(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const GameSession& _game,
        const BestStageRecord& _record, bool _statsStorageError)
    {
        DrawRunDetails(_renderer, _font, _app, CaptureRunResult(_game),
            _record, true, _statsStorageError);
    }

    void DrawLastRunStatistics(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const LastRunStatisticsRecord& _lastRun,
        const BestStageRecord& _record)
    {
        if (_lastRun.GetLastRun())
        {
            DrawRunDetails(_renderer, _font, _app, *_lastRun.GetLastRun(),
                _record, false, _lastRun.HasStorageError());
            return;
        }

        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;
        DrawCenteredText(_renderer, _font, "LAST RUN",
            centerX, centerY + 182.0f, 60.0f, HighlightColor);
        DrawCenteredText(_renderer, _font, "NO COMPLETED RUN YET",
            centerX, centerY + 37.0f, 27.0f, SubtleTextColor);
        if (_lastRun.HasStorageError())
        {
            DrawCenteredText(_renderer, _font, "STATS FILE ERROR",
                centerX, centerY - 12.0f, 20.0f, HighlightColor);
        }
        const ButtonBounds back = ResultButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, "BACK", back,
            back.Contains(_app.GetMouseX(), _app.GetMouseY()));
    }

    void DrawCountdownScreen(Renderer2D& _renderer, const Font& _font,
        const Application& _app, int _number, bool _showGo)
    {
        // The maze must not be rendered before the game clock starts.
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;

        DrawCenteredText(_renderer, _font,
            _showGo ? "START!" : "GET READY",
            centerX, centerY + 100.0f, 32.0f, SubtleTextColor);

        if (_showGo)
        {
            DrawCenteredText(_renderer, _font, "GO!",
                centerX, centerY - 45.0f, 112.0f, HighlightColor);
            return;
        }

        char countdown[8] = {};
        std::snprintf(countdown, sizeof(countdown), "%d", _number);
        DrawCenteredText(_renderer, _font, countdown,
            centerX, centerY - 45.0f, 112.0f, HighlightColor);

        constexpr float segmentGap = 12.0f;
        constexpr float segmentHeight = 6.0f;
        const int totalSegments = GameTuning::Transition::CountdownSeconds;
        const float segmentWidth = std::min(50.0f,
            200.0f / static_cast<float>(totalSegments));
        const float totalWidth = segmentWidth * static_cast<float>(totalSegments)
            + segmentGap * static_cast<float>(totalSegments - 1);
        const float startX = centerX - totalWidth * 0.5f;
        const int activeSegments = totalSegments + 1 - _number;

        for (int i = 0; i < totalSegments; ++i)
        {
            _renderer.DrawRect(startX + static_cast<float>(i) *
                (segmentWidth + segmentGap), centerY - 113.0f,
                segmentWidth, segmentHeight,
                i < activeSegments ? HighlightColor : ButtonColor);
        }
    }

    void DrawStageClearNotice(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const StageClearNotice& _notice)
    {
        if (!_notice.IsVisible())
        {
            return;
        }

        const float opacity = _notice.GetOpacity();
        const float panelWidth = std::min(380.0f,
            static_cast<float>(_app.GetWidth()) - 24.0f);
        if (panelWidth <= 0.0f)
        {
            return;
        }

        const float panelHeight = 84.0f;
        const float panelX = (static_cast<float>(_app.GetWidth()) - panelWidth) * 0.5f;
        const float panelY = std::max(0.0f,
            static_cast<float>(_app.GetHeight()) - 184.0f);

        // Only this compact notice is translucent; never obscure the whole maze.
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.07f, 0.10f, 0.15f, opacity * 0.86f });
        _renderer.DrawRect(panelX, panelY + panelHeight - 3.0f,
            panelWidth, 3.0f, { 1.0f, 0.85f, 0.3f, opacity });

        const StageClearReward& reward = _notice.GetReward();
        char title[96] = {};
        if (reward.newPersonalBest)
        {
            std::snprintf(title, sizeof(title), "NEW PERSONAL BEST!");
        }
        else
        {
            std::snprintf(title, sizeof(title), "STAGE %llu CLEAR!",
                static_cast<unsigned long long>(reward.completedStage));
        }
        DrawCenteredText(_renderer, _font, title,
            panelX + panelWidth * 0.5f, panelY + 46.0f, 25.0f,
            { 1.0f, 0.85f, 0.3f, opacity });

        char earned[128] = {};
        std::snprintf(earned, sizeof(earned), "STAGE %llu    +%.1fs TIME",
            static_cast<unsigned long long>(reward.completedStage),
            static_cast<double>(reward.grantedTimeSeconds));
        DrawCenteredText(_renderer, _font, earned,
            panelX + panelWidth * 0.5f, panelY + 19.0f, 19.0f,
            { 0.96f, 0.97f, 1.0f, opacity });
    }

    void DrawTimeOrbPickupNotice(Renderer2D& _renderer, const Font& _font,
        const Application& _app, float _grantedSeconds, float _remainingSeconds)
    {
        if (_remainingSeconds <= 0.0f)
        {
            return;
        }

        const float opacity = std::clamp(_remainingSeconds / 0.2f, 0.0f, 1.0f);
        constexpr float panelWidth = 272.0f;
        constexpr float panelHeight = 43.0f;
        const float panelX = (static_cast<float>(_app.GetWidth()) - panelWidth) * 0.5f;
        const float panelY = static_cast<float>(_app.GetHeight()) - 119.0f;
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.07f, 0.17f, 0.19f, opacity * 0.88f });

        char pickupText[96] = {};
        std::snprintf(pickupText, sizeof(pickupText), "TIME ORB  +%.1fs",
            static_cast<double>(_grantedSeconds));
        DrawCenteredText(_renderer, _font, pickupText,
            panelX + panelWidth * 0.5f, panelY + 11.0f, 23.0f,
            { 0.12f, 0.95f, 0.92f, opacity });
    }

    void DrawRecallNotice(Renderer2D& _renderer, const Font& _font,
        const Application& _app, std::string_view _message, float _remainingSeconds)
    {
        if (_remainingSeconds <= 0.0f)
        {
            return;
        }
        const float opacity = std::clamp(_remainingSeconds / 0.2f, 0.0f, 1.0f);
        constexpr float panelWidth = 260.0f;
        constexpr float panelHeight = 43.0f;
        const float panelX = (static_cast<float>(_app.GetWidth()) - panelWidth) * 0.5f;
        const float panelY = static_cast<float>(_app.GetHeight()) - 169.0f;
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.19f, 0.08f, 0.24f, opacity * 0.89f });
        DrawCenteredText(_renderer, _font, _message,
            panelX + panelWidth * 0.5f, panelY + 11.0f, 23.0f,
            { 0.94f, 0.74f, 1.0f, opacity });
    }

    void DrawWallBreakNotice(Renderer2D& _renderer, const Font& _font,
        const Application& _app, std::string_view _message, float _remainingSeconds)
    {
        if (_remainingSeconds <= 0.0f)
        {
            return;
        }
        const float opacity = std::clamp(_remainingSeconds / 0.2f, 0.0f, 1.0f);
        constexpr float panelWidth = 262.0f;
        constexpr float panelHeight = 43.0f;
        const float panelX = (static_cast<float>(_app.GetWidth()) - panelWidth) * 0.5f;
        const float panelY = static_cast<float>(_app.GetHeight()) - 219.0f;
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.27f, 0.13f, 0.06f, opacity * 0.89f });
        DrawCenteredText(_renderer, _font, _message,
            panelX + panelWidth * 0.5f, panelY + 11.0f, 23.0f,
            { 1.0f, 0.77f, 0.37f, opacity });
    }

    void DrawTeleportNotice(Renderer2D& _renderer, const Font& _font,
        const Application& _app, std::string_view _message, float _remainingSeconds)
    {
        if (_remainingSeconds <= 0.0f)
        {
            return;
        }
        const float opacity = std::clamp(_remainingSeconds / 0.2f, 0.0f, 1.0f);
        constexpr float panelWidth = 262.0f;
        constexpr float panelHeight = 43.0f;
        const float panelX = (static_cast<float>(_app.GetWidth()) - panelWidth) * 0.5f;
        const float panelY = static_cast<float>(_app.GetHeight()) - 269.0f;
        _renderer.DrawRect(panelX, panelY, panelWidth, panelHeight,
            { 0.24f, 0.08f, 0.14f, opacity * 0.88f });
        DrawCenteredText(_renderer, _font, _message,
            panelX + panelWidth * 0.5f, panelY + 11.0f, 23.0f,
            { 1.0f, 0.69f, 0.82f, opacity });
    }

    struct MazeViewLayout
    {
        float tileSize = 0.0f;
        float startX = 0.0f;
        float startY = 0.0f;
    };

    MazeViewLayout GetMazeViewLayout(const Application& _app, const Maze& _maze)
    {
        constexpr float margin = 32.0f;
        constexpr float hudHeight = 130.0f;
        const float availableWidth = static_cast<float>(_app.GetWidth()) - margin * 2.0f;
        const float availableHeight =
            static_cast<float>(_app.GetHeight()) - margin * 2.0f - hudHeight;
        const float tileSize = std::max(1.0f, std::floor(std::min(
            availableWidth / static_cast<float>(_maze.GetTileWidth()),
            availableHeight / static_cast<float>(_maze.GetTileHeight()))));
        const float mazeWidth = static_cast<float>(_maze.GetTileWidth()) * tileSize;
        const float mazeHeight = static_cast<float>(_maze.GetTileHeight()) * tileSize;
        return { tileSize,
            (static_cast<float>(_app.GetWidth()) - mazeWidth) * 0.5f,
            (static_cast<float>(_app.GetHeight()) - hudHeight - mazeHeight) * 0.5f };
    }

    void DrawGameplay(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const GameSession& _game,
        const BestStageRecord& _record, const PlayerMovementVisual& _visual,
        const TeleportSelection& _teleportSelection)
    {
        const Stage& stage = _game.GetStage();
        const Maze& maze = stage.GetMaze();
        const MazeEndpoints& endpoints = stage.GetEndpoints();
        const VisualCellPosition playerPosition = _visual.GetPosition();

        const float margin = 32.0f;
        const float availableWidth = static_cast<float>(_app.GetWidth()) - margin * 2.0f;
        const MazeViewLayout layout = GetMazeViewLayout(_app, maze);
        const float tileSize = layout.tileSize;
        const float startX = layout.startX;
        const float startY = layout.startY;

        for (int y = 0; y < maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < maze.GetTileWidth(); ++x)
            {
                if (maze.GetTile(x, y) == TileType::Wall)
                {
                    _renderer.DrawRect(startX + x * tileSize, startY + y * tileSize,
                        tileSize, tileSize,
                        maze.IsOuterWall(x, y) ? OuterWallColor : WallColor);
                }
            }
        }

        if (_teleportSelection.IsActive())
        {
            // Soft highlighted squares show valid destinations; there is no
            // pause or wall/path modification while aiming.
            for (int y = 0; y < maze.GetCellHeight(); ++y)
            {
                for (int x = 0; x < maze.GetCellWidth(); ++x)
                {
                    if (stage.CanTeleportTo({ x, y }))
                    {
                        const float inset = tileSize * 0.13f;
                        _renderer.DrawRect(
                            startX + (x * 2 + 1) * tileSize + inset,
                            startY + (y * 2 + 1) * tileSize + inset,
                            tileSize - 2.0f * inset, tileSize - 2.0f * inset,
                            { 1.0f, 0.33f, 0.65f, 0.26f });
                    }
                }
            }
        }

        _renderer.DrawRect(
            startX + (endpoints.start.x * 2 + 1) * tileSize,
            startY + (endpoints.start.y * 2 + 1) * tileSize,
            tileSize, tileSize, StartColor);

        _renderer.DrawRect(
            startX + (endpoints.goal.x * 2 + 1) * tileSize,
            startY + (endpoints.goal.y * 2 + 1) * tileSize,
            tileSize, tileSize, GoalColor);

        // Show the Orb from the start: a contrasting cyan square with a
        // bright core, visible even on small tiles without a new font asset.
        if (stage.GetTimeOrbPosition())
        {
            const CellPosition orb = *stage.GetTimeOrbPosition();
            const float orbX = startX + static_cast<float>(orb.x * 2 + 1) * tileSize;
            const float orbY = startY + static_cast<float>(orb.y * 2 + 1) * tileSize;
            const float inset = tileSize * 0.08f;
            _renderer.DrawRect(orbX + inset, orbY + inset,
                tileSize - inset * 2.0f, tileSize - inset * 2.0f, TimeOrbColor);
            const float coreSize = tileSize * 0.38f;
            const float coreInset = (tileSize - coreSize) * 0.5f;
            _renderer.DrawRect(orbX + coreInset, orbY + coreInset,
                coreSize, coreSize, TimeOrbCoreColor);
        }

        // Recall uses a distinct violet marker and remains visible from spawn.
        if (stage.GetRecallPosition())
        {
            const CellPosition recall = *stage.GetRecallPosition();
            const float itemX = startX + static_cast<float>(recall.x * 2 + 1) * tileSize;
            const float itemY = startY + static_cast<float>(recall.y * 2 + 1) * tileSize;
            const float inset = tileSize * 0.08f;
            _renderer.DrawRect(itemX + inset, itemY + inset,
                tileSize - inset * 2.0f, tileSize - inset * 2.0f, RecallColor);
            const float coreSize = tileSize * 0.35f;
            const float coreInset = (tileSize - coreSize) * 0.5f;
            _renderer.DrawRect(itemX + coreInset, itemY + coreInset,
                coreSize, coreSize, RecallCoreColor);
        }

        // Permanent wall-break item: orange, clearly different from the
        // cyan Time Orb and violet Recall; visible from stage start.
        if (stage.GetWallBreakPosition())
        {
            const CellPosition item = *stage.GetWallBreakPosition();
            const float itemX = startX + static_cast<float>(item.x * 2 + 1) * tileSize;
            const float itemY = startY + static_cast<float>(item.y * 2 + 1) * tileSize;
            const float inset = tileSize * 0.08f;
            _renderer.DrawRect(itemX + inset, itemY + inset,
                tileSize - inset * 2.0f, tileSize - inset * 2.0f, WallBreakColor);
            const float coreSize = tileSize * 0.4f;
            const float coreInset = (tileSize - coreSize) * 0.5f;
            _renderer.DrawRect(itemX + coreInset, itemY + coreInset,
                coreSize, coreSize, WallBreakCoreColor);
        }

        if (stage.GetTeleportPosition())
        {
            const CellPosition item = *stage.GetTeleportPosition();
            const float itemX = startX + static_cast<float>(item.x * 2 + 1) * tileSize;
            const float itemY = startY + static_cast<float>(item.y * 2 + 1) * tileSize;
            const float inset = tileSize * 0.08f;
            _renderer.DrawRect(itemX + inset, itemY + inset,
                tileSize - inset * 2.0f, tileSize - inset * 2.0f, TeleportColor);
            const float coreSize = tileSize * 0.35f;
            const float coreInset = (tileSize - coreSize) * 0.5f;
            _renderer.DrawRect(itemX + coreInset, itemY + coreInset,
                coreSize, coreSize, TeleportCoreColor);
        }

        // White-pink selection outline includes the player's own cancel Cell.
        if (_teleportSelection.IsActive())
        {
            const CellPosition cursor = _teleportSelection.GetSelectedPosition();
            const float cx = startX + (cursor.x * 2 + 1) * tileSize;
            const float cy = startY + (cursor.y * 2 + 1) * tileSize;
            const float border = std::max(1.0f, tileSize * 0.14f);
            _renderer.DrawRect(cx, cy, tileSize, border, TeleportCoreColor);
            _renderer.DrawRect(cx, cy + tileSize - border, tileSize, border, TeleportCoreColor);
            _renderer.DrawRect(cx, cy, border, tileSize, TeleportCoreColor);
            _renderer.DrawRect(cx + tileSize - border, cy, border, tileSize, TeleportCoreColor);
        }

        const float playerSize = tileSize * 0.7f;
        const float playerOffset = (tileSize - playerSize) * 0.5f;
        _renderer.DrawRect(
            startX + (playerPosition.x * 2 + 1) * tileSize + playerOffset,
            startY + (playerPosition.y * 2 + 1) * tileSize + playerOffset,
            playerSize, playerSize, PlayerColor);

        const int recallCount =
            _game.GetInventory().GetCount(ActiveItemType::Recall);
        char recallHud[96] = {};
        std::snprintf(recallHud, sizeof(recallHud), "SPACE  RECALL %d/%d",
            recallCount, ActiveItemInventory::GetCapacity(ActiveItemType::Recall));
        _renderer.DrawText(_font, recallHud,
            margin, static_cast<float>(_app.GetHeight()) - 96.0f,
            19.0f, RecallColor);

        const int wallBreakCount =
            _game.GetInventory().GetCount(ActiveItemType::WallBreak);
        char wallBreakHud[96] = {};
        std::snprintf(wallBreakHud, sizeof(wallBreakHud), "Q + ARROWS  BREAK %d/%d",
            wallBreakCount, ActiveItemInventory::GetCapacity(ActiveItemType::WallBreak));
        const float wallHudWidth = _renderer.MeasureTextWidth(_font, wallBreakHud, 19.0f);
        _renderer.DrawText(_font, wallBreakHud,
            static_cast<float>(_app.GetWidth()) - margin - wallHudWidth,
            static_cast<float>(_app.GetHeight()) - 96.0f, 19.0f, WallBreakColor);

        char teleportHud[100] = {};
        std::snprintf(teleportHud, sizeof(teleportHud), "T  TELEPORT %d/%d",
            _game.GetInventory().GetCount(ActiveItemType::Teleport),
            ActiveItemInventory::GetCapacity(ActiveItemType::Teleport));
        DrawCenteredText(_renderer, _font, teleportHud,
            static_cast<float>(_app.GetWidth()) * 0.5f,
            static_cast<float>(_app.GetHeight()) - 96.0f,
            19.0f, TeleportColor);
        if (_teleportSelection.IsActive())
        {
            DrawCenteredText(_renderer, _font,
                _teleportSelection.IsAtOrigin()
                    ? "ARROWS SELECT    T CANCEL"
                    : "ARROWS SELECT    T CONFIRM",
                static_cast<float>(_app.GetWidth()) * 0.5f,
                static_cast<float>(_app.GetHeight()) - 150.0f,
                17.0f, TeleportCoreColor);
        }

        const float labelY = static_cast<float>(_app.GetHeight()) - 40.0f;
        const float textHeight = 28.0f;

        char stageText[64] = {};
        std::snprintf(stageText, sizeof(stageText), "STAGE %llu",
            static_cast<unsigned long long>(stage.GetNumber()));
        _renderer.DrawText(_font, stageText, margin, labelY, textHeight, TextColor);

        char timeText[64] = {};
        std::snprintf(timeText, sizeof(timeText), "TIME %.1fs", _game.GetRemainingTime());
        DrawCenteredText(_renderer, _font, timeText,
            static_cast<float>(_app.GetWidth()) * 0.5f,
            labelY, textHeight, TextColor);

        char bestText[80] = {};
        std::snprintf(bestText, sizeof(bestText), "BEST %llu",
            static_cast<unsigned long long>(_record.GetBest()));
        const float bestWidth = _renderer.MeasureTextWidth(_font, bestText, textHeight);
        _renderer.DrawText(_font, bestText,
            static_cast<float>(_app.GetWidth()) - margin - bestWidth,
            labelY, textHeight, TextColor);
        if (_record.HasStorageError())
        {
            _renderer.DrawText(_font, "RECORD SAVE ERROR", margin, 16.0f,
                18.0f, HighlightColor);
        }

        const float ratio = std::clamp(
            _game.GetRemainingTime() / _game.GetInitialTime(), 0.0f, 1.0f);
        const float timerY = static_cast<float>(_app.GetHeight()) - 69.0f;

        _renderer.DrawRect(margin, timerY, availableWidth, 9.0f, TimerBackground);
        _renderer.DrawRect(margin, timerY, availableWidth * ratio, 9.0f,
            TimerForeground);
    }
}

static int RunGame(const char* _argv0)
{
    Application app;
    if (!app.Initialize("MazeRunner"))
    {
        return -1;
    }

    Renderer2D renderer;
    if (!renderer.Initialize(app.GetWidth(), app.GetHeight()))
    {
        return -1;
    }

    Font hudFont;
    const std::filesystem::path fontPath =
        GetExecutableDirectory(_argv0) /
        "assets" / "fonts" / "AnonymousPro-Regular.ttf";
    const std::string fontPathUtf8 = fontPath.string();
    if (!hudFont.Load(fontPathUtf8.c_str(), 48.0f))
    {
        std::fprintf(stderr, "Failed to load HUD font: %s\n", fontPath.string().c_str());
        return -1;
    }

    BestStageRecord bestRecord;
    LastRunStatisticsRecord lastRunRecord;
    bool showingStatistics = false;
    bool bestNoticeShownThisRun = false;
    GameFlow flow;
    HeldMovementInput movementInput;
    HeldMovementInput teleportCursorInput;
    TeleportSelection teleportSelection;
    PlayerMovementVisual movementVisual;
    StageClearNotice stageClearNotice;
    float orbNoticeRemainingSeconds = 0.0f;
    float orbNoticeGrantedSeconds = 0.0f;
    float recallNoticeRemainingSeconds = 0.0f;
    std::string recallNoticeText;
    float wallBreakNoticeRemainingSeconds = 0.0f;
    std::string wallBreakNoticeText;
    float teleportNoticeRemainingSeconds = 0.0f;
    std::string teleportNoticeText;

    while (app.IsRunning())
    {
        app.BeginFrame();

        const ScreenState inputState = flow.GetState();
        // Update gameplay before input, so timeout wins over moving onto Goal.
        flow.Update(app.GetDeltaTime());
        // Capture Game Over once, before Flow releases the GameSession on
        // return to main menu. Results remain available after a restart.
        if (inputState == ScreenState::Playing
            && flow.GetState() == ScreenState::Result && flow.GetSession())
        {
            lastRunRecord.SaveRun(*flow.GetSession());
        }
        // Rendering advances continuously between logical steps.
        if (inputState == ScreenState::Playing && flow.GetState() == ScreenState::Playing)
        {
            movementVisual.Update(app.GetDeltaTime());
        }
        stageClearNotice.Update(app.GetDeltaTime());
        if (flow.GetState() == ScreenState::Playing)
        {
            orbNoticeRemainingSeconds =
                std::max(0.0f, orbNoticeRemainingSeconds - app.GetDeltaTime());
            recallNoticeRemainingSeconds =
                std::max(0.0f, recallNoticeRemainingSeconds - app.GetDeltaTime());
            wallBreakNoticeRemainingSeconds =
                std::max(0.0f, wallBreakNoticeRemainingSeconds - app.GetDeltaTime());
            teleportNoticeRemainingSeconds =
                std::max(0.0f, teleportNoticeRemainingSeconds - app.GetDeltaTime());
        }
        if (flow.GetState() == ScreenState::MainMenu
            || flow.GetState() == ScreenState::Result)
        {
            stageClearNotice.Clear();
            orbNoticeRemainingSeconds = 0.0f;
            recallNoticeRemainingSeconds = 0.0f;
            wallBreakNoticeRemainingSeconds = 0.0f;
            teleportNoticeRemainingSeconds = 0.0f;
        }

        if (inputState == ScreenState::MainMenu
            && flow.GetState() == ScreenState::MainMenu)
        {
            if (showingStatistics)
            {
                if (IsClicked(app, ResultButton(app.GetWidth(), app.GetHeight()))
                    || app.IsKeyPressed(Key::ESCAPE))
                {
                    showingStatistics = false;
                }
            }
            else if (IsClicked(app, UpperButton(app.GetWidth(), app.GetHeight())))
            {
                flow.StartGame();
                bestNoticeShownThisRun = false;
                orbNoticeRemainingSeconds = 0.0f;
                recallNoticeRemainingSeconds = 0.0f;
                wallBreakNoticeRemainingSeconds = 0.0f;
                teleportNoticeRemainingSeconds = 0.0f;
            }
            else if (IsClicked(app, LowerButton(app.GetWidth(), app.GetHeight())))
            {
                showingStatistics = true;
            }
            else if (IsClicked(app, MainMenuExitButton(app.GetWidth(), app.GetHeight())))
            {
                app.RequestClose();
            }
        }
        else if (inputState == ScreenState::Playing
            && flow.GetState() == ScreenState::Playing)
        {
            if (app.IsKeyPressed(Key::ESCAPE))
            {
                flow.Pause();
            }
            else if (teleportSelection.IsActive())
            {
                // Timer keeps running, but arrows select a Cell only.
                movementInput.RequireRelease();
                if (app.IsKeyPressed(Key::T))
                {
                    if (teleportSelection.IsAtOrigin())
                    {
                        teleportSelection.Cancel(); // No item consumed.
                        teleportCursorInput.RequireRelease();
                        teleportNoticeText = "TELEPORT CANCELLED";
                        teleportNoticeRemainingSeconds = 1.0f;
                    }
                    else if (flow.TryTeleport(teleportSelection.GetSelectedPosition()))
                    {
                        teleportSelection.Cancel();
                        teleportCursorInput.RequireRelease();
                        movementVisual.Reset(
                            flow.GetSession()->GetStage().GetPlayer().GetPosition());
                        teleportNoticeText = "TELEPORT USED";
                        teleportNoticeRemainingSeconds = 1.0f;
                        const GameSession& currentGame = *flow.GetSession();
                        if (currentGame.WasTimeOrbCollectedOnLastMove())
                        {
                            orbNoticeGrantedSeconds = currentGame.GetLastTimeOrbGrantedSeconds();
                            orbNoticeRemainingSeconds =
                                GameTuning::TimeOrb::PickupNoticeSeconds;
                        }
                        if (currentGame.WasRecallCollectedOnLastMove())
                        {
                            recallNoticeText = "RECALL +1";
                            recallNoticeRemainingSeconds = 1.0f;
                        }
                        if (currentGame.WasWallBreakCollectedOnLastMove())
                        {
                            wallBreakNoticeText = "WALL BREAK +1";
                            wallBreakNoticeRemainingSeconds = 1.0f;
                        }
                        if (currentGame.WasTeleportCollectedOnLastMove())
                        {
                            teleportNoticeText = "TELEPORT +1";
                            teleportNoticeRemainingSeconds = 1.0f;
                        }
                    }
                }
                else
                {
                    const Stage& stage = flow.GetSession()->GetStage();
                    const CellPosition cursor = teleportSelection.GetSelectedPosition();
                    const MovementAvailability available =
                    {
                        teleportSelection.CanSelect(stage, { cursor.x, cursor.y + 1 }),
                        teleportSelection.CanSelect(stage, { cursor.x, cursor.y - 1 }),
                        teleportSelection.CanSelect(stage, { cursor.x - 1, cursor.y }),
                        teleportSelection.CanSelect(stage, { cursor.x + 1, cursor.y })
                    };
                    const MovementButtons buttons =
                    {
                        app.IsKeyDown(Key::UP),
                        app.IsKeyDown(Key::DOWN),
                        app.IsKeyDown(Key::LEFT),
                        app.IsKeyDown(Key::RIGHT)
                    };
                    const CellPosition direction =
                        teleportCursorInput.Update(app.GetDeltaTime(), buttons, available);
                    if (direction != CellPosition{ 0, 0 })
                    {
                        teleportSelection.TryMoveCursor(stage, direction);
                    }
                }
            }
            else if (app.IsKeyPressed(Key::T)
                && flow.GetSession()->GetInventory().GetCount(ActiveItemType::Teleport) > 0)
            {
                if (teleportSelection.Begin(flow.GetSession()->GetStage(),
                    flow.GetSession()->GetInventory()))
                {
                    movementInput.RequireRelease();
                    teleportCursorInput.RequireRelease();
                }
            }
            else if (app.IsKeyPressed(Key::SPACE)
                && flow.TryUseItem(ActiveItemType::Recall))
            {
                movementVisual.Reset(flow.GetSession()->GetStage().GetPlayer().GetPosition());
                movementInput.RequireRelease();
                recallNoticeText = "RECALL USED";
                recallNoticeRemainingSeconds = 1.0f;
            }
            else if (app.IsKeyDown(Key::Q))
            {
                // Hold Q and press exactly one arrow. Either order works:
                // Q while holding an arrow, or an arrow while holding Q.
                // The held Q suppresses ordinary movement and prevents
                // repeatedly spending items on one continuous key hold.
                movementInput.RequireRelease();
                const bool up = app.IsKeyDown(Key::UP);
                const bool down = app.IsKeyDown(Key::DOWN);
                const bool left = app.IsKeyDown(Key::LEFT);
                const bool right = app.IsKeyDown(Key::RIGHT);
                const int held = static_cast<int>(up) + static_cast<int>(down)
                    + static_cast<int>(left) + static_cast<int>(right);
                const bool fresh = app.IsKeyPressed(Key::Q)
                    || app.IsKeyPressed(Key::UP) || app.IsKeyPressed(Key::DOWN)
                    || app.IsKeyPressed(Key::LEFT) || app.IsKeyPressed(Key::RIGHT);
                if (held == 1 && fresh)
                {
                    const CellPosition direction = up ? CellPosition{ 0, 1 }
                        : down ? CellPosition{ 0, -1 }
                        : left ? CellPosition{ -1, 0 }
                        : CellPosition{ 1, 0 };
                    if (flow.TryUseItem(ActiveItemType::WallBreak, direction))
                    {
                        wallBreakNoticeText = "WALL BROKEN";
                        wallBreakNoticeRemainingSeconds = 1.0f;
                    }
                }
            }
            else
            {
                const MovementButtons buttons =
                {
                    app.IsKeyDown(Key::UP),
                    app.IsKeyDown(Key::DOWN),
                    app.IsKeyDown(Key::LEFT),
                    app.IsKeyDown(Key::RIGHT)
                };

                const Stage& activeStage = flow.GetSession()->GetStage();
                const CellPosition currentPosition = activeStage.GetPlayer().GetPosition();
                const Maze& maze = activeStage.GetMaze();
                const MovementAvailability available =
                {
                    maze.CanMove(currentPosition, { currentPosition.x, currentPosition.y + 1 }),
                    maze.CanMove(currentPosition, { currentPosition.x, currentPosition.y - 1 }),
                    maze.CanMove(currentPosition, { currentPosition.x - 1, currentPosition.y }),
                    maze.CanMove(currentPosition, { currentPosition.x + 1, currentPosition.y })
                };
                const CellPosition direction =
                    movementInput.Update(app.GetDeltaTime(), buttons, available);

                if (direction != CellPosition{ 0, 0 })
                {
                    const GameSession& previousGame = *flow.GetSession();
                    const std::uint64_t previousClears = previousGame.GetCompletedStages();
                    const float previousTime = previousGame.GetRemainingTime();

                    flow.TryMove(direction);

                    const GameSession& currentGame = *flow.GetSession();
                    if (currentGame.WasWallBreakCollectedOnLastMove())
                    {
                        wallBreakNoticeText = "WALL BREAK +1";
                        wallBreakNoticeRemainingSeconds = 1.0f;
                    }
                    if (currentGame.WasTeleportCollectedOnLastMove())
                    {
                        teleportNoticeText = "TELEPORT +1";
                        teleportNoticeRemainingSeconds = 1.0f;
                    }
                    if (currentGame.WasRecallCollectedOnLastMove())
                    {
                        recallNoticeText = "RECALL +1";
                        recallNoticeRemainingSeconds = 1.0f;
                    }
                    if (currentGame.WasTimeOrbCollectedOnLastMove())
                    {
                        orbNoticeGrantedSeconds =
                            currentGame.GetLastTimeOrbGrantedSeconds();
                        orbNoticeRemainingSeconds =
                            GameTuning::TimeOrb::PickupNoticeSeconds;
                    }
                    if (currentGame.GetCompletedStages() > previousClears)
                    {
                        // The next maze is already playable. This is visual feedback only.
                        const bool improvedRecord =
                            bestRecord.TryUpdate(currentGame.GetCompletedStages());
                        const bool firstNewBestThisRun =
                            improvedRecord && !bestNoticeShownThisRun;
                        if (firstNewBestThisRun)
                        {
                            bestNoticeShownThisRun = true;
                        }
                        recallNoticeRemainingSeconds = 0.0f;
                        wallBreakNoticeRemainingSeconds = 0.0f;
                        teleportNoticeRemainingSeconds = 0.0f;
                        stageClearNotice.Show({
                            currentGame.GetCompletedStages(),
                            std::max(0.0f, currentGame.GetRemainingTime() - previousTime),
                            firstNewBestThisRun
                        });
                        // A new maze is already active; never animate from the old goal.
                        movementVisual.Reset(currentGame.GetStage().GetPlayer().GetPosition());
                        // Holding a direction must not move into the next maze.
                        movementInput.RequireRelease();
                    }
                    else
                    {
                        const CellPosition nextPosition =
                            currentGame.GetStage().GetPlayer().GetPosition();
                        if (nextPosition != currentPosition)
                        {
                            movementVisual.BeginMove(currentPosition, nextPosition);
                        }
                    }
                }
            }
        }
        else if (inputState == ScreenState::Paused
            && flow.GetState() == ScreenState::Paused)
        {
            if (app.IsKeyPressed(Key::ESCAPE)
                || IsClicked(app, UpperButton(app.GetWidth(), app.GetHeight())))
            {
                flow.Resume();
            }
            else if (IsClicked(app, LowerButton(app.GetWidth(), app.GetHeight())))
            {
                flow.ReturnToMainMenu();
            }
        }
        else if (inputState == ScreenState::Result
            && flow.GetState() == ScreenState::Result)
        {
            if (IsClicked(app, ResultButton(app.GetWidth(), app.GetHeight())))
            {
                flow.ReturnToMainMenu();
            }
        }

        if (inputState != ScreenState::Playing
            || flow.GetState() != ScreenState::Playing)
        {
            // Pause, result, fade and countdown all require a fresh key release.
            movementInput.RequireRelease();
            teleportCursorInput.RequireRelease();
            teleportSelection.Cancel();
            // Pause / countdown / restart must never retain an in-flight animation.
            if (flow.GetSession())
            {
                movementVisual.Reset(flow.GetSession()->GetStage().GetPlayer().GetPosition());
            }
            else
            {
                movementVisual.Reset({ 0, 0 });
            }
        }

        renderer.BeginFrame();
        renderer.DrawRect(0.0f, 0.0f,
            static_cast<float>(app.GetWidth()), static_cast<float>(app.GetHeight()),
            BackgroundColor);

        const ScreenState visibleState = flow.GetVisibleState();
        if (visibleState == ScreenState::MainMenu)
        {
            if (showingStatistics && flow.GetState() == ScreenState::MainMenu)
            {
                DrawLastRunStatistics(renderer, hudFont, app,
                    lastRunRecord, bestRecord);
            }
            else
            {
                DrawMainMenu(renderer, hudFont, app, bestRecord);
            }
        }
        else if (visibleState == ScreenState::Paused)
        {
            DrawPausedMenu(renderer, hudFont, app);
        }
        else if (visibleState == ScreenState::Result && flow.GetSession())
        {
            DrawResult(renderer, hudFont, app, *flow.GetSession(),
                bestRecord, lastRunRecord.HasStorageError());
        }
        else if (visibleState == ScreenState::Countdown
            || visibleState == ScreenState::Go)
        {
            const bool showGo = flow.GetState() == ScreenState::Go;
            // Fade-in previews the initial "3" screen without revealing the maze.
            const int number = flow.GetState() == ScreenState::FadingIn
                ? GameTuning::Transition::CountdownSeconds
                : flow.GetCountdownNumber();
            DrawCountdownScreen(renderer, hudFont, app, number, showGo);
        }
        else if (visibleState == ScreenState::Playing && flow.GetSession())
        {
            DrawGameplay(renderer, hudFont, app, *flow.GetSession(), bestRecord,
                movementVisual, teleportSelection);
            DrawStageClearNotice(renderer, hudFont, app, stageClearNotice);
            DrawTimeOrbPickupNotice(renderer, hudFont, app,
                orbNoticeGrantedSeconds, orbNoticeRemainingSeconds);
            DrawRecallNotice(renderer, hudFont, app, recallNoticeText,
                recallNoticeRemainingSeconds);
            DrawWallBreakNotice(renderer, hudFont, app, wallBreakNoticeText,
                wallBreakNoticeRemainingSeconds);
            DrawTeleportNotice(renderer, hudFont, app, teleportNoticeText,
                teleportNoticeRemainingSeconds);
        }

        const float fadeAlpha = flow.GetFadeAlpha();
        if (fadeAlpha > 0.0f)
        {
            renderer.DrawRect(0.0f, 0.0f,
                static_cast<float>(app.GetWidth()),
                static_cast<float>(app.GetHeight()),
                { 0.0f, 0.0f, 0.0f, fadeAlpha });
        }

        app.EndFrame();
    }

    return 0;
}

#ifdef _WIN32
// A Windows GUI executable must expose WinMain instead of the console entry point.
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return RunGame(nullptr);
}
#else
int main(int argc, char** argv)
{
    return RunGame(argc > 0 ? argv[0] : nullptr);
}
#endif
