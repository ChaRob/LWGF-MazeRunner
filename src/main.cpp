#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/stage/Stage.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
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

    // A fixed value may be passed to mt19937 when reproducing a stage sequence.
    std::random_device randomDevice;
    std::mt19937 seedEngine(randomDevice());

    Stage stage(1, seedEngine());

    const float margin = 32.0f;

    const Color wallColor = { 0.8f, 0.8f, 0.8f, 1.0f };
    const Color startColor = { 0.2f, 0.8f, 0.2f, 1.0f };
    const Color goalColor = { 0.8f, 0.2f, 0.2f, 1.0f };
    const Color playerColor = { 0.2f, 0.5f, 1.0f, 1.0f };

    std::cout << "Stage " << stage.GetNumber()
              << " (" << stage.GetMaze().GetCellWidth() << "x"
              << stage.GetMaze().GetCellHeight() << ")\n";

    while (app.IsRunning())
    {
        app.BeginFrame();

        if (app.IsKeyPressed(Key::ESCAPE))
        {
            app.RequestClose();
            continue;
        }

        CellPosition direction = { 0, 0 };

        // LWGF's rendering coordinate convention has positive Y upward.
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

        if ((direction.x != 0 || direction.y != 0) &&
            stage.TryMovePlayer(direction))
        {
            std::cout << "Stage " << stage.GetNumber() << " Clear!\n";

            if (stage.GetNumber() == std::numeric_limits<std::uint64_t>::max())
            {
                std::cout << "Stage counter limit reached.\n";
                app.RequestClose();
                continue;
            }

            // Create the next maze and reset both endpoints and player.
            stage = Stage(stage.GetNumber() + 1, seedEngine());

            std::cout << "Stage " << stage.GetNumber()
                      << " (" << stage.GetMaze().GetCellWidth() << "x"
                      << stage.GetMaze().GetCellHeight() << ")\n";
        }

        const Maze& maze = stage.GetMaze();
        const MazeEndpoints& endpoints = stage.GetEndpoints();
        const CellPosition& playerPosition = stage.GetPlayer().GetPosition();

        // Recalculate layout for the current stage, including size increases.
        const float availableWidth =
            static_cast<float>(app.GetWidth()) - margin * 2.0f;
        const float availableHeight =
            static_cast<float>(app.GetHeight()) - margin * 2.0f;

        const float tileSize = std::floor(std::min(
            availableWidth / static_cast<float>(maze.GetTileWidth()),
            availableHeight / static_cast<float>(maze.GetTileHeight())
        ));

        const float mazeWidth = static_cast<float>(maze.GetTileWidth()) * tileSize;
        const float mazeHeight = static_cast<float>(maze.GetTileHeight()) * tileSize;

        const float startX =
            (static_cast<float>(app.GetWidth()) - mazeWidth) * 0.5f;
        const float startY =
            (static_cast<float>(app.GetHeight()) - mazeHeight) * 0.5f;

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
            tileSize,
            tileSize,
            startColor
        );

        renderer.DrawRect(
            startX + (endpoints.goal.x * 2 + 1) * tileSize,
            startY + (endpoints.goal.y * 2 + 1) * tileSize,
            tileSize,
            tileSize,
            goalColor
        );

        const float playerSize = tileSize * 0.7f;
        const float playerOffset = (tileSize - playerSize) * 0.5f;

        renderer.DrawRect(
            startX + (playerPosition.x * 2 + 1) * tileSize + playerOffset,
            startY + (playerPosition.y * 2 + 1) * tileSize + playerOffset,
            playerSize,
            playerSize,
            playerColor
        );

        app.EndFrame();
    }

    return 0;
}
