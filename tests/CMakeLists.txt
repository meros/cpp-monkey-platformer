add_executable(test_mechanics test_mechanics.cpp)
target_link_libraries(test_mechanics PRIVATE leafwind_core)
add_test(NAME Mechanics COMMAND test_mechanics WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
