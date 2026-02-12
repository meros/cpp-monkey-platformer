# Unit tests (no GUI dependencies needed)
add_executable(test_tmatrix test_tmatrix.cpp ../TMatrix.cpp ../Point.cpp)
target_include_directories(test_tmatrix PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME TMatrixTests COMMAND test_tmatrix)

add_executable(test_point test_point.cpp ../Point.cpp)
target_include_directories(test_point PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME PointTests COMMAND test_point)

# Asset presence test (runs from source dir so data/ is available)
add_executable(test_assets test_assets.cpp)
target_include_directories(test_assets PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME AssetTests COMMAND test_assets WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

# Sprite loading test (requires SFML + display)
add_executable(test_sprites test_sprites.cpp ../Sprite.cpp)
target_include_directories(test_sprites PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(test_sprites PRIVATE sfml-system sfml-window sfml-graphics)
if(XVFB_RUN)
  add_test(
    NAME SpriteTests
    COMMAND ${XVFB_RUN} -s "-screen 0 800x600x24" $<TARGET_FILE:test_sprites>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  set_tests_properties(SpriteTests PROPERTIES
    LABELS "integration"
    TIMEOUT 15
  )
endif()

# Physics integration test (no GPU needed, tests gameplay invariants)
add_test(
  NAME PhysicsIntegration
  COMMAND ${CMAKE_BINARY_DIR}/monkey_game --test-physics
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
set_tests_properties(PhysicsIntegration PROPERTIES
  LABELS "integration"
  TIMEOUT 30
)

# Rendering smoke test (requires xvfb, verifies the game renders something)
find_program(XVFB_RUN xvfb-run)
if(XVFB_RUN)
  add_test(
    NAME RenderingSmokeTest
    COMMAND ${XVFB_RUN} -s "-screen 0 800x600x24"
      ${CMAKE_BINARY_DIR}/monkey_game
      --screenshot ${CMAKE_BINARY_DIR}/smoke_test.png 5
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  set_tests_properties(RenderingSmokeTest PROPERTIES
    LABELS "integration"
    TIMEOUT 30
  )
endif()

# Visual regression test (requires xvfb-run and imagemagick)
find_program(XVFB_RUN xvfb-run)
if(XVFB_RUN)
  add_test(
    NAME VisualRegression
    COMMAND ${CMAKE_SOURCE_DIR}/tests/visual_test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
  set_tests_properties(VisualRegression PROPERTIES
    LABELS "visual"
    TIMEOUT 30
  )
endif()

# Verify no mario sprites are used in Player code
add_test(
  NAME NoMarioSprites
  COMMAND ${CMAKE_COMMAND} -E env bash -c
    "! grep -q 'mario_' ${CMAKE_SOURCE_DIR}/Player.cpp"
)
set_tests_properties(NoMarioSprites PROPERTIES
  LABELS "qa"
)

# Static analysis via cppcheck (run with: cmake --build build --target cppcheck)
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
  add_custom_target(cppcheck
    COMMAND ${CPPCHECK}
      --enable=warning,performance,portability
      --language=c++
      --std=c++17
      --suppress=missingIncludeSystem
      --quiet
      ${CMAKE_SOURCE_DIR}/*.cpp ${CMAKE_SOURCE_DIR}/*.h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck static analysis"
  )
endif()
