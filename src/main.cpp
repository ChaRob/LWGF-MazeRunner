#include <framework/Application.h>
#include <framework/Renderer2D.h>

#include "game/maze/MazeGenerator.h"
#include "game/maze/MazeEndpointGenerator.h"
#include "game/player/Player.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>

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

    // Maze 생성
    constexpr std::uint32_t seed = 24685;
    constexpr float goalMinimumDistanceRatio = 0.9f;

    Maze maze = MazeGenerator::Generate(
        25,
        25,
        seed
    );

    // Start / Goal 생성
    MazeEndpoints endpoints = MazeEndpointGenerator::Generate(
        maze,
        seed ^ 0x9E3779B9u,
        goalMinimumDistanceRatio
    );

    // Player 생성
    Player player(endpoints.start);

    bool reachedGoal = false;

    // Maze 화면 배치
    const float margin = 32.0f;

    const float availableWidth =
        static_cast<float>(app.GetWidth()) - margin * 2.0f;

    const float availableHeight =
        static_cast<float>(app.GetHeight()) - margin * 2.0f;

    const float tileWidth =
        availableWidth / static_cast<float>(maze.GetTileWidth());

    const float tileHeight =
        availableHeight / static_cast<float>(maze.GetTileHeight());

    const float tileSize =
        std::floor(std::min(tileWidth, tileHeight));

    const float mazeWidth =
        static_cast<float>(maze.GetTileWidth()) * tileSize;

    const float mazeHeight =
        static_cast<float>(maze.GetTileHeight()) * tileSize;

    const float startX =
        (static_cast<float>(app.GetWidth()) - mazeWidth) * 0.5f;

    const float startY =
        (static_cast<float>(app.GetHeight()) - mazeHeight) * 0.5f;

    // Start / Goal Tile 좌표
    const int startTileX = endpoints.start.x * 2 + 1;
    const int startTileY = endpoints.start.y * 2 + 1;

    const int goalTileX = endpoints.goal.x * 2 + 1;
    const int goalTileY = endpoints.goal.y * 2 + 1;

    // 렌더링 색상
    const Color wallColor =
    {
        0.8f, 0.8f, 0.8f, 1.0f
    };

    const Color startColor =
    {
        0.2f, 0.8f, 0.2f, 1.0f
    };

    const Color goalColor =
    {
        0.8f, 0.2f, 0.2f, 1.0f
    };

    const Color playerColor =
    {
        0.2f, 0.5f, 1.0f, 1.0f
    };

    // Game Loop
    while (app.IsRunning())
    {
        app.BeginFrame();

        // ESC: 종료
        if (app.IsKeyPressed(Key::ESCAPE))
        {
            app.RequestClose();
            continue;
        }

        // Player 입력 처리
        if (!reachedGoal)
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

            // 실제 이동 처리
            if (direction.x != 0 || direction.y != 0)
            {
                player.TryMove(maze, direction);

                // Goal 도달 확인
                if (player.GetPosition() == endpoints.goal)
                {
                    reachedGoal = true;

                    std::cout
                        << "Stage Clear!" << std::endl;
                }
            }
        }

        renderer.BeginFrame();

        // 벽 렌더링
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

        // Start 렌더링
        renderer.DrawRect(
            startX + startTileX * tileSize,
            startY + startTileY * tileSize,
            tileSize,
            tileSize,
            startColor
        );

        // Goal 렌더링
        renderer.DrawRect(
            startX + goalTileX * tileSize,
            startY + goalTileY * tileSize,
            tileSize,
            tileSize,
            goalColor
        );

        // Player 렌더링
        const CellPosition& playerPosition =
            player.GetPosition();

        const int playerTileX =
            playerPosition.x * 2 + 1;

        const int playerTileY =
            playerPosition.y * 2 + 1;

        const float playerSize = tileSize * 0.7f;
        const float playerOffset =
            (tileSize - playerSize) * 0.5f;

        renderer.DrawRect(
            startX + playerTileX * tileSize + playerOffset,
            startY + playerTileY * tileSize + playerOffset,
            playerSize,
            playerSize,
            playerColor
        );

        app.EndFrame();
    }

    return 0;
}
