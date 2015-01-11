cmake_minimum_required(VERSION 2.6)
project(cpp-monkey-platformer)

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

# Detect and add SFML
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")
#Find any version 2.X of SFML
#See the FindSFML.cmake file for additional details and instructions

message(${CMAKE_MODULE_PATH})

add_executable(${EXECUTABLE_NAME} ${HEADERS} ${SOURCES})

find_package(SFML REQUIRED system window graphics network audio)
if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
  target_link_libraries(${EXECUTABLE_NAME} ${SFML_LIBRARIES})
endif()

find_package(BOX2D REQUIRED)
if(SFML_FOUND)
  include_directories(${BOX2D_INCLUDE_DIR})
  target_link_libraries(${EXECUTABLE_NAME} ${BOX2D_LIBRARIES})
endif()



