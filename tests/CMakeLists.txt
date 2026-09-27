add_executable(test_mechanics test_mechanics.cpp)
target_link_libraries(test_mechanics PRIVATE leafwind_core)
add_test(NAME Mechanics COMMAND test_mechanics WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_levels test_levels.cpp RouteBot.h)
target_link_libraries(test_levels PRIVATE leafwind_core)
add_test(NAME LevelRoutes COMMAND test_levels WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
