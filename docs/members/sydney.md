# Sydney — Engine Architecture / ECS

Secondary responsibility: Repository Design/Maintenance.

## What is here

World stores transforms with stable IDs; this is not a full ECS.

- Public code: `engine/include/engine/architecture.hpp`
- Implementation: `engine/src/architecture.cpp`
- Example tests: `tests/architecture_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^architecture$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **Component storage and entity lifecycle** — Add typed component add/get/remove operations, reject stale IDs, and test deletion cleanup. Proposed effort: 6 hours.

2. **Fixed-step loop and scenes** — Run a fixed simulation step with a capped accumulator; test scene reset and input consumption across multiple ticks. Proposed effort: 6 hours.

3. **Events and integrated world** — Agree event payloads with gameplay/physics/networking; demonstrate create, update, destroy without dangling references. Proposed effort: 5 hours.

## Coordinate with

Blake, D'Artagnan, Henry; Arvin for shared types and lifetime review.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
