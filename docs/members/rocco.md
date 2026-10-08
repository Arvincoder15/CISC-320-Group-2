# Rocco — Game AI

Secondary responsibility: Team Manager.

## What is here

PatrolController is a reusable left/right state machine for optional moving hazards or NPCs. Both main characters are human-controlled; a companion AI is not required.

- Public code: `engine/include/engine/ai.hpp`
- Implementation: `engine/src/ai.cpp`
- Example tests: `tests/ai_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^ai$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Agent decision interface** — Agree reusable patrol/state-machine interfaces with Blake; test direction changes at bounds and keep puzzle rules in the game layer. Proposed effort: 4 hours.

2. **Optional patrol hazard** — If approved, use the patrol controller for a moving hazard or NPC in one puzzle room; expose bounds in level data and test predictable transitions. Both player roles remain human-controlled. Proposed effort: 6 hours.

3. **AI evaluation and sprint coordination** — Test decisions for invalid observations and repeatable scenarios; record agreed sprint scope and owners in Jira. Proposed effort: 4 hours.

## Coordinate with

Blake for observation adapter; Sydney for entity access; D'Artagnan for collision queries.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
