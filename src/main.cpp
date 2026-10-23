#include <framework/Application.h>
#include <framework/Font.h>
#include <framework/Renderer2D.h>

#include "game/GameFlow.h"
#include "game/player/HeldMovementInput.h"

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
    constexpr Color StartColor = { 0.2f, 0.8f, 0.2f, 1.0f };
    constexpr Color GoalColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    constexpr Color PlayerColor = { 0.2f, 0.5f, 1.0f, 1.0f };
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

    ButtonBounds ResultButton(int _width, int _height)
    {
        return { _width * 0.5f - 145.0f, _height * 0.5f - 135.0f, 290.0f, 58.0f };
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
        const Application& _app)
    {
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;
        DrawCenteredText(_renderer, _font, "MAZE RUNNER",
            centerX, centerY + 136.0f, 64.0f, HighlightColor);
        DrawCenteredText(_renderer, _font, "ESCAPE THE MAZE BEFORE TIME RUNS OUT",
            centerX, centerY + 88.0f, 21.0f, SubtleTextColor);

        const ButtonBounds start = UpperButton(_app.GetWidth(), _app.GetHeight());
        const ButtonBounds exit = LowerButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, "START", start,
            start.Contains(_app.GetMouseX(), _app.GetMouseY()));
        DrawButton(_renderer, _font, "EXIT", exit,
            exit.Contains(_app.GetMouseX(), _app.GetMouseY()));
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

    void DrawResult(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const GameSession& _game)
    {
        const float centerX = _app.GetWidth() * 0.5f;
        const float centerY = _app.GetHeight() * 0.5f;
        DrawCenteredText(_renderer, _font, "GAME OVER",
            centerX, centerY + 136.0f, 64.0f, HighlightColor);

        char clearedText[96] = {};
        std::snprintf(clearedText, sizeof(clearedText), "STAGES CLEARED  %llu",
            static_cast<unsigned long long>(_game.GetCompletedStages()));
        DrawCenteredText(_renderer, _font, clearedText,
            centerX, centerY + 72.0f, 27.0f, TextColor);

        char scoreText[96] = {};
        std::snprintf(scoreText, sizeof(scoreText), "FINAL SCORE  %llu",
            static_cast<unsigned long long>(_game.GetScore()));
        DrawCenteredText(_renderer, _font, scoreText,
            centerX, centerY + 29.0f, 31.0f, TextColor);

        const ButtonBounds menu = ResultButton(_app.GetWidth(), _app.GetHeight());
        DrawButton(_renderer, _font, "MAIN MENU", menu,
            menu.Contains(_app.GetMouseX(), _app.GetMouseY()));
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

        constexpr float segmentWidth = 50.0f;
        constexpr float segmentGap = 12.0f;
        constexpr float segmentHeight = 6.0f;
        constexpr float totalWidth = segmentWidth * 3.0f + segmentGap * 2.0f;
        const float startX = centerX - totalWidth * 0.5f;
        const int activeSegments = 4 - _number;

        for (int i = 0; i < 3; ++i)
        {
            _renderer.DrawRect(startX + static_cast<float>(i) *
                (segmentWidth + segmentGap), centerY - 113.0f,
                segmentWidth, segmentHeight,
                i < activeSegments ? HighlightColor : ButtonColor);
        }
    }

    void DrawGameplay(Renderer2D& _renderer, const Font& _font,
        const Application& _app, const GameSession& _game)
    {
        const Stage& stage = _game.GetStage();
        const Maze& maze = stage.GetMaze();
        const MazeEndpoints& endpoints = stage.GetEndpoints();
        const CellPosition& playerPosition = stage.GetPlayer().GetPosition();

        const float margin = 32.0f;
        const float hudHeight = 86.0f;
        const float availableWidth = static_cast<float>(_app.GetWidth()) - margin * 2.0f;
        const float availableHeight =
            static_cast<float>(_app.GetHeight()) - margin * 2.0f - hudHeight;
        const float tileSize = std::floor(std::min(
            availableWidth / static_cast<float>(maze.GetTileWidth()),
            availableHeight / static_cast<float>(maze.GetTileHeight())));

        const float mazeWidth = static_cast<float>(maze.GetTileWidth()) * tileSize;
        const float mazeHeight = static_cast<float>(maze.GetTileHeight()) * tileSize;
        const float startX = (static_cast<float>(_app.GetWidth()) - mazeWidth) * 0.5f;
        const float startY =
            (static_cast<float>(_app.GetHeight()) - hudHeight - mazeHeight) * 0.5f;

        for (int y = 0; y < maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < maze.GetTileWidth(); ++x)
            {
                if (maze.GetTile(x, y) == TileType::Wall)
                {
                    _renderer.DrawRect(startX + x * tileSize, startY + y * tileSize,
                        tileSize, tileSize, WallColor);
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

        const float playerSize = tileSize * 0.7f;
        const float playerOffset = (tileSize - playerSize) * 0.5f;
        _renderer.DrawRect(
            startX + (playerPosition.x * 2 + 1) * tileSize + playerOffset,
            startY + (playerPosition.y * 2 + 1) * tileSize + playerOffset,
            playerSize, playerSize, PlayerColor);

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

        char scoreText[80] = {};
        std::snprintf(scoreText, sizeof(scoreText), "SCORE %llu",
            static_cast<unsigned long long>(_game.GetScore()));
        const float scoreWidth = _renderer.MeasureTextWidth(_font, scoreText, textHeight);
        _renderer.DrawText(_font, scoreText,
            static_cast<float>(_app.GetWidth()) - margin - scoreWidth,
            labelY, textHeight, TextColor);

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
    if (!app.Initialize())
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

    GameFlow flow;
    HeldMovementInput movementInput;

    while (app.IsRunning())
    {
        app.BeginFrame();

        const ScreenState inputState = flow.GetState();
        // Update gameplay before input, so timeout wins over moving onto Goal.
        flow.Update(app.GetDeltaTime());

        if (inputState == ScreenState::MainMenu
            && flow.GetState() == ScreenState::MainMenu)
        {
            if (IsClicked(app, UpperButton(app.GetWidth(), app.GetHeight())))
            {
                flow.StartGame();
            }
            else if (IsClicked(app, LowerButton(app.GetWidth(), app.GetHeight())))
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
            else
            {
                const MovementButtons buttons =
                {
                    app.IsKeyDown(Key::UP),
                    app.IsKeyDown(Key::DOWN),
                    app.IsKeyDown(Key::LEFT),
                    app.IsKeyDown(Key::RIGHT)
                };

                const CellPosition direction =
                    movementInput.Update(app.GetDeltaTime(), buttons);

                if (direction != CellPosition{ 0, 0 })
                {
                    const std::uint64_t previousStage =
                        flow.GetSession()->GetStage().GetNumber();
                    flow.TryMove(direction);

                    if (flow.GetSession()->GetStage().GetNumber() != previousStage)
                    {
                        // Holding a direction must not move into the next maze.
                        movementInput.RequireRelease();
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
        }

        renderer.BeginFrame();
        renderer.DrawRect(0.0f, 0.0f,
            static_cast<float>(app.GetWidth()), static_cast<float>(app.GetHeight()),
            BackgroundColor);

        const ScreenState visibleState = flow.GetVisibleState();
        if (visibleState == ScreenState::MainMenu)
        {
            DrawMainMenu(renderer, hudFont, app);
        }
        else if (visibleState == ScreenState::Paused)
        {
            DrawPausedMenu(renderer, hudFont, app);
        }
        else if (visibleState == ScreenState::Result && flow.GetSession())
        {
            DrawResult(renderer, hudFont, app, *flow.GetSession());
        }
        else if (visibleState == ScreenState::Countdown
            || visibleState == ScreenState::Go)
        {
            const bool showGo = flow.GetState() == ScreenState::Go;
            // Fade-in previews the initial "3" screen without revealing the maze.
            const int number = flow.GetState() == ScreenState::FadingIn
                ? 3 : flow.GetCountdownNumber();
            DrawCountdownScreen(renderer, hudFont, app, number, showGo);
        }
        else if (visibleState == ScreenState::Playing && flow.GetSession())
        {
            DrawGameplay(renderer, hudFont, app, *flow.GetSession());
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
