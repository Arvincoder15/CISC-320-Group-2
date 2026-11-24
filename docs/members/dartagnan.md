# D'Artagnan — Physics & Collision

Secondary responsibility: Technical Writer.

## Implementation

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

The SDL campaign uses `PhysicsSystem` for platform collisions and contact state. The headless game module uses a flat-floor simulation.

- Public code: `engine/include/engine/physics.hpp`
- Implementation: `engine/src/physics.cpp`
- Tests: `tests/physics_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R '^physics$' --output-on-failure` (use double quotes in Windows shells).
