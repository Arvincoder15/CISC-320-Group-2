# D'Artagnan — Physics & Collision

Secondary responsibility: Technical Writer.

## What is here

Semi-implicit Euler and strict AABB overlap are implemented as small reference helpers.

- Public code: `engine/include/engine/physics.hpp`
- Implementation: `engine/src/physics.cpp`
- Example tests: `tests/physics_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^physics$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Collision components and layers** — Connect body/collider data to ECS; resolve floors, walls and ceilings, track grounded state, and test landing, edge contact and entity removal. Proposed effort: 6 hours.

2. **Collision events and triggers** — Emit enter/stay/leave events for hazards, exits and pressure plates without elemental game rules in engine physics; test contact cleanup after restart. Proposed effort: 6 hours.

3. **Movement integration and regression** — Agree fixed-step policy, gravity units and speed limits; test restart and document fast-moving collision limitations. Proposed effort: 4 hours.

## Coordinate with

Sydney for components; Blake for platform/trigger semantics; Gunveer for collider data.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
