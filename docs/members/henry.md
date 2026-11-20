# Henry — Networking

Secondary responsibility: Repository Design/Maintenance.

## What is here

Two-player networked play over ENet: connection and disconnect detection, the co-op protocol, and host/client
sync drivers, with a two-process demo. How to use it, test results and open items: [NETWORKING.md](../NETWORKING.md).

- Engine API: `engine/include/engine/networking.hpp` (`Session`, byte reader/writer, in-memory test pair)
- Implementation: `engine/src/networking.cpp`, `engine/src/enet_session.cpp`
- Game protocol and drivers: `games/elemental_coop/include/elemental_coop/net.hpp`, `games/elemental_coop/src/net.cpp`
- Demo: `engine_demo --host <port>` / `engine_demo --join <address> <port>` (`apps/headless_demo.cpp`)
- Tests: `tests/net_session_test.cpp`, `tests/coop_net_test.cpp` (old queue: `tests/networking_test.cpp`)
- Run them: `ctest --test-dir build -C Debug -R "net_session|coop_net|networking" --output-on-failure`

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
