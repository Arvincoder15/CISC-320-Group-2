# Rocco — Game AI

Secondary responsibility: Team Manager.

## What is here

VerticalSteering is a generic threshold decision; an AI opponent remains proposed.

- Public code: `engine/include/engine/ai.hpp`
- Implementation: `engine/src/ai.cpp`
- Example tests: `tests/ai_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^ai$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **Agent decision interface** — Agree observation/action types with Blake; keep engine decisions independent of bird rules and test boundary conditions. Proposed effort: 4 hours.

2. **Optional demonstration opponent** — If scope is approved, map obstacle observations to actions with cooldown; demonstrate an agent using the same gameplay input path. Proposed effort: 6 hours.

3. **AI evaluation and sprint coordination** — Test decisions for invalid observations and repeatable scenarios; record agreed sprint scope and owners in Jira. Proposed effort: 4 hours.

## Coordinate with

Blake for observation adapter; Sydney for entity access; D'Artagnan for collision queries.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
