# Aryaman — Rendering

Secondary responsibility: Archivist.

## What is here

Renderer is an interface; RecordingRenderer copies draw commands for tests. No SDL window exists.

- Public code: `engine/include/engine/rendering.hpp`
- Implementation: `engine/src/rendering.cpp`
- Example tests: `tests/rendering_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^rendering$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **SDL3 window and renderer** — Add an approved pinned SDL3 dependency and RAII backend; show a window and handle initialization errors cleanly. Proposed effort: 6 hours.

2. **Cached sprite rendering** — Draw two sprites sharing one cached texture; validate missing-asset behavior and texture-before-renderer shutdown. Proposed effort: 6 hours.

3. **Camera and draw ordering** — Add camera transforms and layers; demonstrate game world to screen conversion with deterministic command tests. Proposed effort: 5 hours.

## Coordinate with

Arvin for texture handles; Sydney for transforms; Rohan for UI and SDL event ownership.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
