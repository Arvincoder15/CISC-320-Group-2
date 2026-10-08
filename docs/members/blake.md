# Blake — Gameplay Systems

Secondary responsibility: Software Developer/Architect.

## What is here

Game owns one entity, applies a flap action and gravity, and stops on quit. There are no pipes, score, restart, or actual multiplayer.

- Public code: `games/flappy`
- Implementation: `games/flappy/src/game.cpp`
- Example tests: `tests/gameplay_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^gameplay$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **Single-player round** — Add obstacle spawning, collision-driven loss and scoring in games/flappy; use engine components and deterministic tests. Proposed effort: 8 hours.

2. **Restart and tuning** — Implement round states, complete reset and validated tuning data; repeated restart leaves no old entities or score. Proposed effort: 6 hours.

3. **Multiplayer rules and game adapter** — Agree authority, score/win conditions and shared obstacle timing with Henry; integrate two clients using the engine protocol. Proposed effort: 8 hours.

## Coordinate with

All subsystem owners; especially Sydney, D'Artagnan, Henry, and Rohan.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
