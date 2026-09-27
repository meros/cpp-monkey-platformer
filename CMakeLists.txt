cmake_minimum_required(VERSION 3.16)
project(leafwind LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(EXECUTABLE_NAME monkey_game)

find_package(SFML 2.6 COMPONENTS system window graphics audio REQUIRED)
find_package(box2d REQUIRED)

# --- Simulation core: no graphics, used by the game and the headless tests ---
add_library(leafwind_core STATIC
  src/core/Config.h
  src/core/Events.h
  src/core/Input.h
  src/core/Entities.h
  src/core/Level.h src/core/Level.cpp
  src/core/Player.h src/core/Player.cpp
  src/core/World.h src/core/World.cpp
  src/core/ContactListener.h src/core/ContactListener.cpp
  src/core/Save.h src/core/Save.cpp
  src/core/Headless.h src/core/Headless.cpp)
target_include_directories(leafwind_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(leafwind_core PUBLIC box2d::box2d)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(leafwind_core PRIVATE -Wall -Wextra)
endif()

# --- Game executable ---
add_executable(${EXECUTABLE_NAME}
  src/main.cpp
  src/gfx/Palette.h src/gfx/Palette.cpp
  src/gfx/Draw.h src/gfx/Draw.cpp
  src/gfx/Assets.h src/gfx/Assets.cpp
  src/gfx/Particles.h src/gfx/Particles.cpp
  src/gfx/Background.h src/gfx/Background.cpp
  src/gfx/TerrainRenderer.h src/gfx/TerrainRenderer.cpp
  src/gfx/Camera.h src/gfx/Camera.cpp
  src/gfx/Renderer.h src/gfx/Renderer.cpp src/gfx/RenderObjects.cpp
  src/audio/Synth.h src/audio/Synth.cpp
  src/audio/Audio.h src/audio/Audio.cpp
  src/game/InputDevice.h src/game/InputDevice.cpp
  src/game/UI.h src/game/UI.cpp
  src/game/Game.h src/game/Game.cpp)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE leafwind_core
  sfml-system sfml-window sfml-graphics sfml-audio)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${EXECUTABLE_NAME} PRIVATE -Wall -Wextra)
endif()

enable_testing()
add_subdirectory(tests)
