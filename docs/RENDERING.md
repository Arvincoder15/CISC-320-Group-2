# Rendering subsystem

The reusable SDL3 backend implements the existing `Renderer::draw` interface. The
headless `RecordingRenderer` and existing callers remain supported. SDL3 3.2.28
is the same version already pinned in the UI prototype CI. The backend is optional;
the default build does not download SDL or require a display.

## Build and run

With CMake and a C++20 compiler installed:

```sh
cmake -S . -B build-rendering -DGROUP2_BUILD_SDL_RENDERER=ON -DGROUP2_FETCH_SDL=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-rendering --config Debug --parallel
ctest --test-dir build-rendering -C Debug --output-on-failure
```

Run `build-rendering/rendering_demo` (or `build-rendering/Debug/rendering_demo.exe`
for Visual Studio). Assets are copied beside the executable; running from another
working directory is supported. Arrow keys pan, Q/E zoom, Space pauses animations,
R resets the camera, and Escape closes the window. This is a rendering demonstration,
not a replacement for the team's playable UI or game rules.

To use an installed SDL package instead, omit `GROUP2_FETCH_SDL` and supply
`CMAKE_PREFIX_PATH` if needed. Installed shared builds require the SDL library at
runtime. Linux headless machines may need `-DSDL_UNIX_CONSOLE_BUILD=ON` when fetching SDL.

`rendering_demo --smoke-test --capture frame.bmp` renders 60 deterministic frames
and exits. Set `SDL_VIDEODRIVER=dummy` to run without a display. Normal execution
uses the available graphics driver. Pixel tests use the dummy software driver.

## Contracts for teammates

- `SpriteDraw` preserves its original first three aggregate fields. Added fields
  default to native texture dimensions, white tint, layer zero and no rotation.
- Positions are top-left coordinates in pixels (+y downward). Rotation is clockwise
  in degrees about the sprite centre. Source rectangles select atlas regions.
- `draw` validates commands, clears the window, stable-sorts by ascending layer,
  draws, and presents. Equal layers retain submission order. Submit one complete
  frame; UI can use `screen_space=true` and a higher layer.
- An empty asset key means a solid rectangle with an explicit positive size. It
  does not support rotation, source rectangles or flipping; textured sprites do.
- `Camera::position` is the visible world's top-left point. `follow` centres on a
  target; smooth follow and world bounds are application policies. The viewport
  matches the logical window resolution. Resizes preserve aspect ratio with
  letterboxing. `screen_to_world` expects logical coordinates, not raw mouse pixels.
- Texture keys are explicitly registered with `load_texture`. BMP paths are
  canonicalized and share Arvin's `ResourceCache`; aliases reuse a texture. A key
  cannot silently change files. Failed loads are not cached. Missing draw keys and
  invalid geometry throw exceptions rather than hiding errors.
- Portable RGB BMPs use magenta (255,0,255) as transparency. PNG/JPEG decoding is
  not included. Atlas bounds are checked against actual texture dimensions.
- `SpriteAnimation` uses elapsed seconds, supports looping and one-shot playback,
  and returns a row-major source rectangle. Negative/nonfinite time is rejected.
- `tile_draws` accepts a rectangular row-major grid, zero for empty and 1-based
  atlas indices. This is a drawing helper, not a new level-file format; Gunveer's
  importer remains responsible for level data and Tiled flip flags.
- `text_draws` uses a fixed-cell, 16-column printable ASCII BMP atlas. Newlines work;
  unsupported bytes become `?`. Unicode shaping and TrueType are not implemented.
- Conservative culling skips sprites wholly outside the view. Cached textures,
  stable draw ordering and SDL batching avoid repeated image decoding. There is
  no claim of a measured thousands-of-sprites performance target yet.

## Ownership and integration

`SdlRenderer` owns one SDL video reference, a window, a renderer and its texture
cache. Destruction releases textures before renderer/window, then releases its
video reference. Constructor failures clean up partial resources. Texture handles
never leave the backend, so callers cannot accidentally retain them after shutdown.
All calls are on the main thread. The backend does **not** poll events: the
application must have a single event owner and forward input as appropriate.

The demo is the composition root for rendering only. Engine code contains no
fire/water game rules. It illustrates particle effects with ordinary alpha-tinted
draw commands, not a new gameplay subsystem. Advanced shaders, a general particle
simulation, screen shake, and integration into Rohan's UI remain follow-up work.

Review the extended draw-command fields with consumers, the cache lifecycle with
Arvin, event ownership with Rohan, and level adapters with Gunveer. The team has not
yet recorded approval of these proposed integration choices. Jira and Confluence
records must be added using real task IDs; none are fabricated by this change.

## Test coverage

The headless suite exercises camera transforms, following, animation wrapping and
one-shot completion, tile atlas indexing and text layout. SDL pixel tests cover
texture alias caching, failure handling, camera transforms, crop bounds, layer
ordering, tint reset, horizontal flips, rotation, alpha blending and repeated
creation/destruction. The demo smoke test loads actual assets and renders 60 frames.

The generated character/tile images are original procedural demo art. The font is
adapted from the existing prototype atlas with its license retained. Regeneration
is optional: `python tools/generate_rendering_assets.py` requires Pillow.

## Playable game integration

The optional `ui_prototype` is also built when the root SDL renderer option is
enabled. Its existing window, event routing, audio, menus and game rules stay under
the application's ownership. A borrowed `SdlRenderer` submits cached platform
tiles and player sprites into the application's current frame without clearing or
presenting it. Live ECS positions, movement direction, grounded state and animation
time drive the sprites. Pause and restart freeze/reset animation. The application's
camera settings are passed to the renderer, including the fit views for Medium and
Hard. The adapter restores host draw color and blend mode after each submission.

`ui_game_smoke` runs the existing automated Easy, Medium and Hard gameplay routes,
plus live renderer checks for cache reuse, sprite position and pause behavior. Use
`build-rendering/apps/ui_prototype/ui_prototype.exe` on this Windows checkout to
play locally. This game still has no connected multiplayer session; the lobby says
so in its UI. Networking is Henry's separate subsystem.

Implementation and tests were produced with Codex assistance. Review and understand
the code and record assistance according to the course/team policy; this document
does not claim personal diary activity or team approval.
