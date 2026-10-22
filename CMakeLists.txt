cmake_minimum_required(VERSION 3.20)

project(
    MazeRunner
    VERSION 0.0.1
    LANGUAGES C CXX
)

add_subdirectory(
    external/LightWeightGameFramework
)

set(MAZERUNNER_GAME_SOURCES
    src/game/GameSession.cpp
    src/game/GameSession.h
    src/game/GameFlow.cpp
    src/game/GameFlow.h
    src/game/maze/Maze.cpp
    src/game/maze/Maze.h
    src/game/maze/MazeGenerator.cpp
    src/game/maze/MazeGenerator.h
    src/game/maze/CellPosition.h
    src/game/maze/MazeEndpoints.h
    src/game/maze/MazeEndpointGenerator.cpp
    src/game/maze/MazeEndpointGenerator.h
    src/game/player/Player.cpp
    src/game/player/Player.h
    src/game/stage/Stage.cpp
    src/game/stage/Stage.h
)

add_executable(MazeRunner WIN32
    src/main.cpp
    ${MAZERUNNER_GAME_SOURCES}
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

# Bundle the HUD font next to the executable, independent of the launch directory.
set(MAZERUNNER_HUD_FONT
    "${CMAKE_CURRENT_SOURCE_DIR}/external/LightWeightGameFramework/sample/assets/fonts/AnonymousPro-Regular.ttf"
)
if(NOT EXISTS "${MAZERUNNER_HUD_FONT}")
    message(FATAL_ERROR "HUD font missing: ${MAZERUNNER_HUD_FONT}. Run git submodule update --init --recursive.")
endif()
add_custom_command(TARGET MazeRunner POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:MazeRunner>/assets/fonts"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${MAZERUNNER_HUD_FONT}"
        "$<TARGET_FILE_DIR:MazeRunner>/assets/fonts/AnonymousPro-Regular.ttf"
    COMMENT "Copy MazeRunner HUD font to executable assets directory"
    VERBATIM
)

include(CTest)

if(BUILD_TESTING)
    add_executable(MazeRunnerFlowTests
        tests/GameFlowTests.cpp
        ${MAZERUNNER_GAME_SOURCES}
    )
    target_compile_features(MazeRunnerFlowTests PRIVATE cxx_std_20)
    add_test(NAME MazeRunnerFlowTests COMMAND MazeRunnerFlowTests)
endif()
