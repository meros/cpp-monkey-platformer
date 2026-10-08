# cpp-monkey-platformer

A small 2D platformer in C++ where you play a monkey, built on SFML for graphics and Box2D for physics. The original idea for this game was/is a 2d platformer with physics as a gameplay mechanism.

Written 2010–2011, picked up again in 2015, and ported to SFML 2.6 and Box2D 2.4 with tests in 2026.

## Build and run

With Nix:

```sh
nix run github:meros/cpp-monkey-platformer
```

With CMake 3.16 or later, SFML 2.6 and Box2D 2.4 installed:

```sh
cmake -B build
cmake --build build
./build/monkey_game
```

Run the game from the repository root: it loads its assets from `data/` relative to the current directory. Escape closes the window.

`nix develop` gives a shell with the dependencies plus cppcheck, xvfb-run and ImageMagick for the tests.

## Tests

```sh
cd build && ctest --output-on-failure
```

The unit tests cover the matrix and point math and check that the assets exist. `monkey_game --test-physics` runs the physics for 500 frames without a window. The rendering and visual regression tests run only when `xvfb-run` is installed.

## Status

Current status is not much to brag about, there is a room with a rope and some floors. The direction of this project is very uncertain.

## License

0BSD, except `data/smb3_mario_sheet.png` (a Nintendo sprite sheet) and `alleg42.dll` (the Allegro library, under its own license). See `LICENSE`.
