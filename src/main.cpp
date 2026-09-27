#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/maze/MazeGenerator.h"

#include <algorithm>
#include <cmath>

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

	Maze maze = MazeGenerator::Generate(25, 25, 12345);

    const float margin = 32.0f;

    const float availableWidth = static_cast<float>(app.GetWidth()) - margin * 2.0f;
    const float availableHeight = static_cast<float>(app.GetHeight()) - margin * 2.0f;

    const float tileWidth = availableWidth / static_cast<float>(maze.GetTileWidth());
    const float tileHeight = availableHeight / static_cast<float>(maze.GetTileHeight());

    const float tileSize = std::floor(std::min(tileWidth, tileHeight));

    const float mazeWidth = static_cast<float>(maze.GetTileWidth()) * tileSize;
    const float mazeHeight = static_cast<float>(maze.GetTileHeight()) * tileSize;

    const float startX = (static_cast<float>(app.GetWidth()) - mazeWidth) * 0.5f;
    const float startY = (static_cast<float>(app.GetHeight()) - mazeHeight) * 0.5f;

    const Color wallColor = {
        0.8f,
        0.8f,
        0.8f,
        1.0f
    };

    while (app.IsRunning())
    {
        app.BeginFrame();
        renderer.BeginFrame();

        for (int y = 0; y < maze.GetTileHeight(); ++y)
        {
            for (int x = 0; x < maze.GetTileWidth(); ++x)
            {
                if (maze.GetTile(x, y) != TileType::Wall)
                {
                    continue;
                }

                renderer.DrawRect(
                    startX + x * tileSize,
                    startY + y * tileSize,
                    tileSize,
                    tileSize,
                    wallColor
                );
            }
        }

        app.EndFrame();
    }

    return 0;
}