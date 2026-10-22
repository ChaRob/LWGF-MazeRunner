#include <framework/Application.h>
#include <framework/Font.h>
#include <framework/Renderer2D.h>

#include "game/GameSession.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <string>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <random>
#include <stdexcept>

namespace
{
std::filesystem::path GetExecutableDirectory(const char* _argv0)
{
#ifdef _WIN32
    std::wstring executablePath(32768, L'\\0');
    DWORD length = GetModuleFileNameW(nullptr, &executablePath[0],
        static_cast<DWORD>(executablePath.size()));
    if (length == 0 || length >= executablePath.size())
        throw std::runtime_error("Cannot resolve MazeRunner executable directory");
    executablePath.resize(length);
    return std::filesystem::path(executablePath).parent_path();
#else
    return std::filesystem::absolute(_argv0).parent_path();
#endif
}
}

int main(int argc, char** argv)
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
        GetExecutableDirectory(argv[0]) / "assets" / "fonts" / "AnonymousPro-Regular.ttf";
    const std::string fontPathUtf8 = fontPath.string();
    if (!hudFont.Load(fontPathUtf8.c_str(), 48.0f))
    {
        std::fprintf(stderr, "Failed to load HUD font: %s\n", fontPath.string().c_str());
        return -1;
    }

    std::random_device randomDevice;
    GameSession game(randomDevice());

    const float margin = 32.0f;
    const float hudHeight = 86.0f;
    const Color textColor = { 0.96f, 0.97f, 1.0f, 1.0f };
    const Color highlightColor = { 1.0f, 0.85f, 0.3f, 1.0f };
    const Color overlayColor = { 0.06f, 0.08f, 0.11f, 0.94f };


    const Color wallColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    const Color startColor = { 0.2f, 0.8f, 0.2f, 1.0f };
    const Color goalColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    const Color playerColor = { 0.2f, 0.5f, 1.0f, 1.0f };
    const Color timerBackground = { 0.25f, 0.25f, 0.25f, 1.0f };
    const Color timerForeground = { 0.25f, 0.75f, 0.3f, 1.0f };
    const Color gameOverColor = { 0.85f, 0.2f, 0.2f, 1.0f };

    while (app.IsRunning())
    {
        app.BeginFrame();

        if (app.IsKeyPressed(Key::ESCAPE))
        {
            app.RequestClose();
            continue;
        }

        // Always resolve timeout before accepting input for this frame.
        game.Update(app.GetDeltaTime());

        if (game.GetStatus() == GameStatus::Playing)
        {
            CellPosition direction = { 0, 0 };

            if (app.IsKeyPressed(Key::UP))
            {
                direction = { 0, 1 };
            }
            else if (app.IsKeyPressed(Key::DOWN))
            {
                direction = { 0, -1 };
            }
            else if (app.IsKeyPressed(Key::LEFT))
            {
                direction = { -1, 0 };
            }
            else if (app.IsKeyPressed(Key::RIGHT))
            {
                direction = { 1, 0 };
            }

            if (direction.x != 0 || direction.y != 0)
            {
                game.TryMove(direction);
            }
        }

        const Stage& stage = game.GetStage();
        const Maze& maze = stage.GetMaze();
        const MazeEndpoints& endpoints = stage.GetEndpoints();
        const CellPosition& playerPosition = stage.GetPlayer().GetPosition();

        // Leave space above the maze for a temporary, text-free time bar.
        // Reserve space for Stage / Time / Score labels above the maze.
        const float availableWidth =
            static_cast<float>(app.GetWidth()) - margin * 2.0f;
        const float availableHeight =
            static_cast<float>(app.GetHeight()) - margin * 2.0f - hudHeight;

        const float tileSize = std::floor(std::min(
            availableWidth / static_cast<float>(maze.GetTileWidth()),
            availableHeight / static_cast<float>(maze.GetTileHeight())
        ));

        const float mazeWidth = static_cast<float>(maze.GetTileWidth()) * tileSize;
        const float mazeHeight = static_cast<float>(maze.GetTileHeight()) * tileSize;
        const float startX =
            (static_cast<float>(app.GetWidth()) - mazeWidth) * 0.5f;
        const float startY =
            (static_cast<float>(app.GetHeight()) - hudHeight - mazeHeight) * 0.5f;

        renderer.BeginFrame();

        for (int y = 0; y < maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < maze.GetTileWidth(); ++x)
            {
                if (maze.GetTile(x, y) == TileType::Wall)
                {
                    renderer.DrawRect(
                        startX + x * tileSize,
                        startY + y * tileSize,
                        tileSize,
                        tileSize,
                        wallColor
                    );
                }
            }
        }

        renderer.DrawRect(
            startX + (endpoints.start.x * 2 + 1) * tileSize,
            startY + (endpoints.start.y * 2 + 1) * tileSize,
            tileSize, tileSize, startColor
        );

        renderer.DrawRect(
            startX + (endpoints.goal.x * 2 + 1) * tileSize,
            startY + (endpoints.goal.y * 2 + 1) * tileSize,
            tileSize, tileSize, goalColor
        );

        const float playerSize = tileSize * 0.7f;
        const float playerOffset = (tileSize - playerSize) * 0.5f;

        renderer.DrawRect(
            startX + (playerPosition.x * 2 + 1) * tileSize + playerOffset,
            startY + (playerPosition.y * 2 + 1) * tileSize + playerOffset,
            playerSize, playerSize, playerColor
        );

        // HUD labels use the bottom-left screen coordinate system.
        const float labelY = static_cast<float>(app.GetHeight()) - 40.0f;
        const float textHeight = 28.0f;

        char stageText[64] = {};
        std::snprintf(stageText, sizeof(stageText), "STAGE %llu",
            static_cast<unsigned long long>(stage.GetNumber()));
        renderer.DrawText(hudFont, stageText, margin, labelY, textHeight, textColor);

        char timeText[64] = {};
        std::snprintf(timeText, sizeof(timeText), "TIME %.1fs",
            game.GetRemainingTime());
        const float timeWidth = renderer.MeasureTextWidth(hudFont, timeText, textHeight);
        renderer.DrawText(hudFont, timeText,
            (static_cast<float>(app.GetWidth()) - timeWidth) * 0.5f,
            labelY, textHeight, textColor);

        char scoreText[80] = {};
        std::snprintf(scoreText, sizeof(scoreText), "SCORE %llu",
            static_cast<unsigned long long>(game.GetScore()));
        const float scoreWidth = renderer.MeasureTextWidth(hudFont, scoreText, textHeight);
        renderer.DrawText(hudFont, scoreText,
            static_cast<float>(app.GetWidth()) - margin - scoreWidth,
            labelY, textHeight, textColor);

        const float timerWidth = availableWidth;
        const float ratio = std::clamp(
            game.GetRemainingTime() / game.GetInitialTime(),
            0.0f, 1.0f
        );

        renderer.DrawRect(
            margin, static_cast<float>(app.GetHeight()) - 69.0f,
            timerWidth, 9.0f, timerBackground
        );

        renderer.DrawRect(
            margin, static_cast<float>(app.GetHeight()) - 69.0f,
            timerWidth * ratio, 9.0f,
            game.GetStatus() == GameStatus::GameOver
                ? gameOverColor : timerForeground
        );

        if (game.GetStatus() == GameStatus::GameOver)
        {
            const float centerX = static_cast<float>(app.GetWidth()) * 0.5f;
            const float centerY = static_cast<float>(app.GetHeight()) * 0.5f;

            renderer.DrawRect(
                centerX - 290.0f, centerY - 102.0f,
                580.0f, 210.0f, overlayColor
            );

            const float titleHeight = 48.0f;
            const float titleWidth = renderer.MeasureTextWidth(
                hudFont, "GAME OVER", titleHeight);
            renderer.DrawText(hudFont, "GAME OVER",
                centerX - titleWidth * 0.5f,
                centerY + 52.0f, titleHeight, highlightColor);

            char resultText[96] = {};
            std::snprintf(resultText, sizeof(resultText),
                "CLEARED %llu  SCORE %llu",
                static_cast<unsigned long long>(game.GetCompletedStages()),
                static_cast<unsigned long long>(game.GetScore()));

            const float resultHeight = 26.0f;
            const float resultWidth = renderer.MeasureTextWidth(
                hudFont, resultText, resultHeight);
            renderer.DrawText(hudFont, resultText,
                centerX - resultWidth * 0.5f,
                centerY - 6.0f, resultHeight, textColor);

            const float hintHeight = 20.0f;
            const float hintWidth = renderer.MeasureTextWidth(
                hudFont, "ESC TO EXIT", hintHeight);
            renderer.DrawText(hudFont, "ESC TO EXIT",
                centerX - hintWidth * 0.5f,
                centerY - 50.0f, hintHeight, textColor);
        }

        app.EndFrame();
    }

    return 0;
}
