# Leafwind tests (docs/DESIGN.md section 8). Run: cd build && ctest --output-on-failure

# --- headless unit / simulation tests ---
add_executable(test_assets test_assets.cpp)
add_test(NAME AssetTests COMMAND test_assets WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_save test_save.cpp)
target_link_libraries(test_save PRIVATE leafwind_core)
add_test(NAME SaveTests COMMAND test_save)

add_executable(test_synth test_synth.cpp ${CMAKE_SOURCE_DIR}/src/audio/Synth.cpp)
target_include_directories(test_synth PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_synth PRIVATE sfml-system)
add_test(NAME SynthTests COMMAND test_synth)

add_executable(test_mechanics test_mechanics.cpp)
target_link_libraries(test_mechanics PRIVATE leafwind_core)
add_test(NAME Mechanics COMMAND test_mechanics WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(test_levels test_levels.cpp RouteBot.h)
target_link_libraries(test_levels PRIVATE leafwind_core)
add_test(NAME LevelRoutes COMMAND test_levels WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(LevelRoutes PROPERTIES TIMEOUT 120)

add_test(NAME LevelValidation COMMAND $<TARGET_FILE:monkey_game> --validate-levels
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_test(NAME PhysicsIntegration COMMAND $<TARGET_FILE:monkey_game> --test-physics
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
set_tests_properties(PhysicsIntegration PROPERTIES LABELS "integration" TIMEOUT 60)

# --- tests that need a display (run through xvfb-run when available) ---
find_program(XVFB_RUN xvfb-run)
if(XVFB_RUN)
  add_test(NAME RenderingSmokeTest
           COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24" $<TARGET_FILE:monkey_game>
                   --screenshot 1 30 ${CMAKE_BINARY_DIR}/smoke_test.png
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  set_tests_properties(RenderingSmokeTest PROPERTIES LABELS "integration" TIMEOUT 60)

  add_test(NAME GameFlow
           COMMAND ${XVFB_RUN} -a -s "-screen 0 800x600x24" $<TARGET_FILE:monkey_game> --flow-test
           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  set_tests_properties(GameFlow PROPERTIES LABELS "integration" TIMEOUT 120)

  find_program(IM_COMPARE compare)
  if(IM_COMPARE)
    add_test(NAME VisualRegression
             COMMAND ${CMAKE_SOURCE_DIR}/tests/visual_test.sh $<TARGET_FILE:monkey_game>
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    set_tests_properties(VisualRegression PROPERTIES LABELS "visual" TIMEOUT 180)
  endif()
endif()

# Static analysis via cppcheck (run with: cmake --build build --target cppcheck)
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK} --enable=warning,performance,portability --language=c++ --std=c++17
            --suppress=missingIncludeSystem --quiet -I ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck static analysis")
endif()
