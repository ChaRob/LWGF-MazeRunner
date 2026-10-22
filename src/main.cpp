#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/GameSession.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

int main()
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

    std::random_device randomDevice;
    GameSession game(randomDevice());

    const float margin = 32.0f;

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
        const float hudHeight = 28.0f;
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

        const float timerWidth = availableWidth;
        const float ratio = std::clamp(
            game.GetRemainingTime() / game.GetInitialTime(),
            0.0f, 1.0f
        );

        renderer.DrawRect(
            margin, static_cast<float>(app.GetHeight()) - 20.0f,
            timerWidth, 9.0f, timerBackground
        );

        renderer.DrawRect(
            margin, static_cast<float>(app.GetHeight()) - 20.0f,
            timerWidth * ratio, 9.0f,
            game.GetStatus() == GameStatus::GameOver
                ? gameOverColor : timerForeground
        );

        if (game.GetStatus() == GameStatus::GameOver)
        {
            // Temporary visual status until LWGF supports text rendering.
            renderer.DrawRect(
                margin, static_cast<float>(app.GetHeight()) - 32.0f,
                timerWidth, 6.0f, gameOverColor
            );
        }

        app.EndFrame();
    }

    return 0;
}
