# Henry — Networking

Secondary responsibility: Repository Design/Maintenance.

## What is here

LoopbackTransport is a bounded test queue. No sockets, remote peers, serialization, or multiplayer are implemented.

- Public code: `engine/include/engine/networking.hpp`
- Implementation: `engine/src/networking.cpp`
- Example tests: `tests/networking_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^networking$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Protocol and authority design** — Review cooperative authority and one human per player slot with Blake; define input/state packets, level identity, round generation, bounds, IDs and tick ordering. Proposed effort: 4 hours.

2. **Two-process connection prototype** — Choose transport with team review; connect two local processes, handle disconnects, and document reproducible launch commands. Proposed effort: 8 hours.

3. **Snapshot synchronization** — Synchronize both characters, switches/doors and shared round state between two clients; reject invalid/stale/wrong-slot input and test disconnect plus shared restart. Proposed effort: 8 hours.

## Coordinate with

Sydney for entity mapping; Blake for game state; Arvin for diagnostics; Rohan for lobby UI.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
