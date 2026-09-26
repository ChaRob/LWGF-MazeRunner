#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/maze/MazeGenerator.h"

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

	Maze maze = MazeGenerator::Generate(10, 10, 12345);

    const float tileSize = 24.0f;

    const float mazeWidth = maze.GetTileWidth() * tileSize;
    const float mazeHeight = maze.GetTileHeight() * tileSize;

    const float startX = (app.GetWidth() - mazeWidth) * 0.5f;
    const float startY = (app.GetHeight() - mazeHeight) * 0.5f;

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