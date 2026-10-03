#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/maze/MazeGenerator.h"
#include "game/maze/MazeEndpointGenerator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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

    constexpr std::uint32_t seed = 24685;
    constexpr float GoalMinimumDistanceRatio = 0.9f;

	Maze maze = MazeGenerator::Generate(25, 25, seed);

    MazeEndpoints endpoints = MazeEndpointGenerator::Generate(maze, seed ^ 0x9E3779B9u, GoalMinimumDistanceRatio);

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

    const int startTileX = endpoints.start.x * 2 + 1;
    const int startTileY = endpoints.start.y * 2 + 1;

    const int goalTileX = endpoints.goal.x * 2 + 1;
    const int goalTileY = endpoints.goal.y * 2 + 1;

    const Color wallColor = {
        0.8f,
        0.8f,
        0.8f,
        1.0f
    };

    const Color startColor =
    {
        0.2f,
        0.8f,
        0.2f,
        1.0f
    };

    const Color goalColor =
    {
        0.8f,
        0.2f,
        0.2f,
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

        renderer.DrawRect(
            startX + startTileX * tileSize,
            startY + startTileY * tileSize,
            tileSize,
            tileSize,
            startColor
        );

        renderer.DrawRect(
            startX + goalTileX * tileSize,
            startY + goalTileY * tileSize,
            tileSize,
            tileSize,
            goalColor
        );

        app.EndFrame();
    }

    return 0;
}
