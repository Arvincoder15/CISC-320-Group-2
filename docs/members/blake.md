# Blake — Gameplay Systems

Secondary responsibility: Software Developer/Architect.

## What is here

Game owns two elemental characters with independent walking/jumping, a temporary flat floor, shared loss/restart and a joint exit condition. Hazard and exit contacts are supplied manually through integration seams. There are no rendered levels, working switches/doors or real network connections.

- Public code: `games/elemental_coop`
- Implementation: `games/elemental_coop/src/game.cpp`
- Example tests: `tests/gameplay_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^gameplay$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Cooperative puzzle room** — Build one cooperative room in games/elemental_coop with platforms, elemental hazards, two matching exits and a pressure plate controlling a door; connect contact events and test joint completion. Proposed effort: 8 hours.

2. **Restart and tuning** — Extend restart to reset the entire puzzle: both characters, switches, doors and collectibles; load validated movement tuning and test repeated shared restart. Proposed effort: 6 hours.

3. **Multiplayer rules and game adapter** — Agree player-slot ownership, host authority, shared failure/restart and joint exit rules with Henry; synchronize both characters and puzzle objects through the engine protocol. Proposed effort: 8 hours.

## Coordinate with

All subsystem owners; especially Sydney, D'Artagnan, Henry, and Rohan.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
