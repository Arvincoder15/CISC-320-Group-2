# Rohan — User Interaction & UI

Secondary responsibility: Artist.

## What is here

InputBuffer supports a held movement axis and one-shot actions. Use a separate buffer per player. No SDL polling, menus, HUD or audio exist.

- Public code: `engine/include/engine/interaction.hpp`
- Implementation: `engine/src/interaction.cpp`
- Example tests: `tests/interaction_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^interaction$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **SDL action mapping** — Centralize SDL event polling; map two independent local control sets or one local network slot, clear held movement on focus loss, and test isolated jump actions. Proposed effort: 5 hours.

2. **Menus and HUD** — Implement title, local co-op/host/join, character assignment, shared failure/restart and level-complete UI; show both player states and verify ImGui input capture. Proposed effort: 6 hours.

3. **Audio and settings** — Load approved sound assets through shared infrastructure; add volume validation and clean shutdown with the audio backend. Proposed effort: 6 hours.

## Coordinate with

Aryaman for rendering; Blake for game states; Arvin for resources/settings; Gunveer for ImGui capture.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
