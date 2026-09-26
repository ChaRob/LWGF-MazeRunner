#include <framework/Application.h>
#include "game/maze/MazeGenerator.h"
#include <iostream>

int main()
{
    Application app;

    if (!app.Initialize())
    {
        return -1;
    }

	Maze maze = MazeGenerator::Generate(10, 10, 12345);

    for (int y = 0; y < maze.GetTileHeight(); ++y)
    {
        for (int x = 0; x < maze.GetTileWidth(); ++x)
        {
            const TileType tile = maze.GetTile(x, y);

            std::cout << (
                tile == TileType::Wall
                ? '#'
                : '.'
                );
        }

        std::cout << '\n';
    }

    while (app.IsRunning())
    {
        app.BeginFrame();

        app.EndFrame();
    }

    return 0;
}