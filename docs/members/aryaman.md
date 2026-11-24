# Aryaman — Rendering

Secondary responsibility: Archivist.

## Implementation

Renderer is an interface; RecordingRenderer copies draw commands for tests. The
optional `SdlRenderer` backend now opens a window and renders cached BMP textures,
sprite sheets, layers, camera transforms and bitmap text. See
[rendering setup and contracts](../RENDERING.md) for the standalone demo, tests,
and playable game integration.

- Public code: `engine/include/engine/rendering.hpp`
- Implementation: `engine/src/rendering.cpp`
- Tests: `tests/rendering_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R '^rendering$' --output-on-failure` (use double quotes in Windows shells).
