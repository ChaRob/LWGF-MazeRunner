cmake_minimum_required(VERSION 3.20)

project(
    MazeRunner
    VERSION 0.0.1
    LANGUAGES C CXX
)

add_subdirectory(
    external/LightWeightGameFramework
)

add_executable(
    MazeRunner

    src/main.cpp
    src/game/maze/Maze.cpp
    src/game/maze/Maze.h
    src/game/maze/MazeGenerator.cpp
    src/game/maze/MazeGenerator.h
)

target_compile_features(
    MazeRunner
    PRIVATE
        cxx_std_20
)

target_link_libraries(
    MazeRunner
    PRIVATE
        LWGF::Framework
)