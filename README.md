# Space Invaders

Space Invaders runs on a small C++ engine: an OpenGL 3.3 renderer, packed component arrays, a spatial hash, and a sprite batcher. The normal game is the player, the invader swarm, bullets, and the score. Stress mode is a separate scene.

## Build and run

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/engine_tests
./build/spaceinvaders
./build/spaceinvaders --stress
```

`make.sh` runs the same two cmake commands. `engine_tests` does not open a window. The game binary asks GLFW for an OpenGL 3.3 core context. glfw and glm are fetched by CMake. glad is vendored under `third_party/glad`.

The GL target needs X11 and OpenGL headers. On Debian or Ubuntu that is `libgl1-mesa-dev`, `libx11-dev`, `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`, `libxext-dev`, `libxrender-dev`, `libxfixes-dev`, and `x11proto-dev`. If those are missing, configure with `-DSPACEINVADERS_BUILD_GL=OFF` and build the `engine_tests` target.

Left and right move the ship, space fires, escape quits. F5 toggles stress mode. F1 draws the hash cells that currently hold an AABB.

The window did run here. Mesa llvmpipe reported `OpenGL 4.5 (Core Profile)` for the 3.3 core request, and the 330 shader linked. `glfwSwapInterval(1)` is set. The stress log showed `frame_ms=4.34`, so the swap did not lock the frame at 60 Hz on this machine.

The same stress scene, rebuilt with `SpriteBatch` capacity set to 1, logged `draws=658`. Capacity 512 logged `draws=2` for `entities=640`.

## Layout

- `src/engine/batch.cpp` closes a batch when the texture changes or the range reaches capacity (512 sprites).
- `src/engine/renderer.cpp` is one GLSL 330 shader, one dynamic VBO, one dynamic IBO, and one `glDrawElements` per batch.
- `src/engine/pool.hpp` and `src/engine/world.cpp` keep components in packed arrays. An entity id is a slot index. `src/engine/systems.hpp` walks those arrays.
- `src/engine/spatial_hash.cpp` inserts each AABB into the grid cells it touches and narrowphase-tests only that cell and its neighbors.
- `src/game/sprites.hpp` and `src/game/atlas.cpp` pack the original pixel masks into one atlas.
- `src/game/game.cpp` is the player, invaders, bullets, and score. Hits go through the hash.
- `src/game/stress.cpp` spawns 640 moving sprites and runs the same hash.
- `src/tests/test_ecs.cpp`, `src/tests/test_spatial.cpp`, and `src/tests/test_batch.cpp` are the headless checks. The spatial test moves 640 boxes, checks a 48-body sample against brute force, and requires fewer pair tests than `n*(n-1)/2`.
- `src/main.cpp` owns the window, input, HUD, and the stress log.

## Stress mode

Once a second, stress mode prints the live entity count, the batch count, the draw-call count, the average frame time over that second, and the number of occupied hash cells:

```
stress entities=640 batches=2 draws=2 frame_ms=4.34 hash_cells=56
```

`batches` and `draws` are one counter. `Renderer::end` increments it for each `glDrawElements`. The HUD shows the previous frame in the game font, at the top right: `E` entity count, `B` batches, frame time in milliseconds, and `C` hash cells.
