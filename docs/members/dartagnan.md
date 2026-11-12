# D'Artagnan — Physics & Collision

Secondary responsibility: Technical Writer.

## What is here

A reusable 2D physics system built on the ECS `World`, alongside the original helpers.

- `integrate` / `overlaps`: unchanged semi-implicit Euler and strict AABB overlap helpers. Existing callers (gameplay, AI tests) still use them.
- `Collider`: ECS component with an offset from the `Transform`, a size, `layer`/`mask` bit flags and a `trigger` flag. `bounds()` gives its world-space box. `layers_interact()` requires each collider's mask to accept the other's layer.
- `PhysicsSettings`: global gravity (px/s², default 0, added to each `Body` acceleration), per-axis speed limits, substep size (1/60 s) and count (8), and the grounded-probe skin. `validate()` rejects bad values.
- `PhysicsSystem`: register entities with `add(world, id)`. `step(world, dt)` validates every tracked component before changing anything, then for each substep:
  - integrates bodies;
  - resolves walls (x first), then floors and ceilings, against static colliders (a `Collider` with no `Body`);
  - writes `ContactState` (grounded, supporting `ground` entity, ceiling, wall flags).

  Destroyed entities are dropped automatically. Moving bodies do not block each other.
- Triggers: `events()` lists enter/stay/leave between moving bodies and trigger colliders, in (body, trigger) ID order. A trigger crossed partway through a step is still reported. Destroying or unregistering an entity produces leave. `reset()` drops contacts without emitting leave, for restarts. Moving triggers (a trigger with a `Body`) pass through solids.
- `query(world, area, mask)`: registered colliders overlapping an area, for AI and gameplay checks.
- Known limitations (also listed in the header):
  - Overlap tests are discrete, so a body moving further per substep than its own size plus a solid's thickness can tunnel through it.
  - Bodies that start inside a solid are not pushed out.
  - No moving platforms; static solids are assumed not to move.
  - Every body is checked against every solid each substep.
  - Transform rotation and scale are ignored.
- Build note: `engine_physics` now links `engine_architecture` in `CMakeLists.txt` (shared file; flag for review).

### Task status

| Task | State |
|---|---|
| 1. Collision components and layers | Implemented and unit-tested in the engine; `elemental_coop` still uses its flat floor until Blake connects `PhysicsSystem` |
| 2. Collision events and triggers | Implemented and unit-tested, including cleanup after restart; hazards/exits/plates are not yet wired to game rules |
| 3. Movement integration and regression | Substep policy, gravity units and speed limits implemented and tested; the fixed-step accumulator remains Sydney's (SYDNEY-02) and the values above still need team agreement |

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
