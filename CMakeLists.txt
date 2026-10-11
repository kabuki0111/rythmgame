cmake_minimum_required(VERSION 3.24)
project(RhythmGame LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# SDL3 をソースから取得して静的リンク（DLL のコピーが不要になる）
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.18
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(RhythmGame
    src/main.cpp
    src/Config.h
    src/AudioClock.h src/AudioClock.cpp
    src/Chart.h src/Chart.cpp
    src/Judge.h src/Judge.cpp
    src/Game.h src/Game.cpp src/GameRender.cpp
)
target_link_libraries(RhythmGame PRIVATE SDL3::SDL3)

if(MSVC)
    target_compile_options(RhythmGame PRIVATE /utf-8 /W4)
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT RhythmGame)
else()
    target_compile_options(RhythmGame PRIVATE -Wall -Wextra)
endif()

# 実行ファイルの隣に assets/ をコピー
add_custom_command(TARGET RhythmGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:RhythmGame>/assets)
