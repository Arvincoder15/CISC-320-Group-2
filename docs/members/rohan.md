# Rohan — User Interaction & UI

Secondary responsibility: Artist.

## What is here

InputBuffer consumes one-shot actions. No SDL polling, menus, HUD or audio exist.

- Public code: `engine/include/engine/interaction.hpp`
- Implementation: `engine/src/interaction.cpp`
- Example tests: `tests/interaction_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^interaction$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **SDL action mapping** — Centralize SDL event polling; map keys to actions, handle quit/focus and test that one press is consumed once. Proposed effort: 5 hours.

2. **Menus and HUD** — Implement title/pause/game-over UI and score display from game state; verify input capture with ImGui. Proposed effort: 6 hours.

3. **Audio and settings** — Load approved sound assets through shared infrastructure; add volume validation and clean shutdown with the audio backend. Proposed effort: 6 hours.

## Coordinate with

Aryaman for rendering; Blake for game states; Arvin for resources/settings; Gunveer for ImGui capture.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
