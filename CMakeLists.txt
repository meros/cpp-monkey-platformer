cmake_minimum_required(VERSION 3.16)
project(cpp-monkey-platformer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_NAME monkey_game)

set(HEADERS
  World.h
  TMatrix.h
  Sprite.h
  Rope.h
  Point.h
  Player.h
  IRotateable.h
  IPositionable.h
  IDrawable.h
  DummyDrawable.h
  ContactListener.h
  Block.h
  Bridge.h)

set(SOURCES
  main.cpp
  World.cpp
  TMatrix.cpp
  Sprite.cpp
  Rope.cpp
  Point.cpp
  Player.cpp
  DummyDrawable.cpp
  ContactListener.cpp
  Block.cpp
  Bridge.cpp)

add_executable(${EXECUTABLE_NAME} ${HEADERS} ${SOURCES})

# SFML 2.6 (config mode)
find_package(SFML 2.6 COMPONENTS system window graphics network audio REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE sfml-system sfml-window sfml-graphics sfml-network sfml-audio)

# Box2D 2.4 (config mode)
find_package(box2d REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE box2d::box2d)

# Tests
enable_testing()
add_subdirectory(tests)
