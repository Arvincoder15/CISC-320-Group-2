# Rocco — Game AI

Secondary responsibility: Team Manager.

## Implemented

The reusable AI module provides the original horizontal PatrolController, cyclic world-space patrol routes, nearest-visible-target selection, patrol/chase/attack/search/stunned decisions, attack cooldowns, damage-triggered stun, four-direction grid A*, conservative tile line of sight, and waypoint following.

Public interfaces are in engine/include/engine/ai.hpp. Logic and explanatory comments are in engine/src/ai.cpp. The AI suite is in tests/ai_test.cpp and the standalone Terminal demonstration is in apps/ai_demo.cpp. These interfaces are local proposals for owner/consumer review.

## Build and try it

From the repository root, using this Mac's local CMake installation:

```sh
./.venv-build/bin/cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
./.venv-build/bin/cmake --build build --parallel
./build/ai_demo
./.venv-build/bin/ctest --test-dir build -R '^ai$' --output-on-failure
./.venv-build/bin/ctest --test-dir build --output-on-failure
```

On machines with CMake on PATH, use cmake and ctest directly.

The demo prints an A* detour around a wall, reports blocked direct visibility, then feeds scripted observations through the enemy state machine. The AI suite also drives waypoints through World components and the existing physics integrator.

## Read the code in this order

1. AiConfig, AiObservation and AiDecision in the header describe input, tuning and output.
2. AiController::update in the implementation shows validation, damage interruption, target selection, combat, search and patrol.
3. test_decisions shows concrete examples and expected results.
4. NavigationGrid and find_path show obstacle representation and shortest-path search.
5. WaypointFollower and test_movement_integration show how destinations connect to movement.

## Behaviour rules

Each agent owns an AiController. Observations borrow target snapshots for one call. Gameplay supplies only eligible living targets, excluding the agent and friendly actors. Visibility comes from has_line_of_sight or a future physics ray query.

Detection covers all directions inside detection_range. The nearest visible target wins, with the lowest entity ID breaking exact distance ties. A target within attack_range receives an attack request if the cooldown has expired. At most one request is emitted per update, including after a long frame. Zero-dt updates may update observations/decisions but cannot emit attacks.

Losing a target starts a timed search toward its last visible position. The agent waits there until the search expires, then resumes its cyclic patrol. A visible target interrupts search. Damage sets a stun timer and immediately suppresses movement and attacks. Repeated damage restarts stun. Cooldown advances during stun; search time pauses during stun. After recovery, the controller reevaluates the current observations. Gameplay handles health, death, knockback and actual damage.

Empty patrol routes stand still. Multiple coincident/reached points are skipped with a bounded loop. reset clears state, target memory, timers and patrol progress.

AiConfig permits different enemy/NPC types through tuning. For example, a guard can use short detection and slow movement while a pursuer uses longer detection and faster movement. Set engage_targets=false for a peaceful patrol. The team should choose names and gameplay values.

## Navigation contract

NavigationGrid owns a rectangular obstacle map. A* uses four-direction unit-cost movement and includes start and goal in the returned route. Empty output means no route or invalid/blocked endpoints. The algorithm uses Manhattan distance and a stable tie-break.

Line of sight connects tile centres and blocks every touched obstacle, including corner-adjacent tiles. The current example uses one grid for sight and movement; consumers can supply separate grids when transparent/solid rules differ.

Grid cells and world pixels are separate types. Level Tools must define tile dimensions and map origin. A typical cell centre is origin + (cell + 0.5) * tile_size. Convert the path into Vec2 world destinations before constructing WaypointFollower. Replan when the destination or obstacle data changes. The follower advances points within its arrival radius and returns the next point; movement should clamp travel to the waypoint to avoid overshoot.

The grid models a point agent moving on a flat plane. Collision clearance requires marking cells according to agent size. Platform navigation needs traversable edges describing support surfaces, jumps and drops. A* runs synchronously with memory proportional to the grid; use bounded map sizes and avoid replanning every frame for every NPC.

## Proposed integration sequence

1. Level Tools supplies obstacle data and coordinate mapping.
2. Gameplay reads an NPC position and eligible target snapshots from World.
3. Physics or grid sight checks set target visibility.
4. AiController::update selects a state, destination and optional attack request.
5. Navigation plans a route to the destination and WaypointFollower chooses the next point.
6. Movement sets velocity, applies physics/collision and writes the actual resulting position.
7. Gameplay consumes attack requests, validates combat rules and reports damage events next tick.

The existing cooperative game still uses its human player controls. The new AI demo and tests demonstrate the reusable module separately.

## Coordination and remaining work

- Blake reviews observation/decision semantics, target eligibility, attack handling and whether NPCs belong in the cooperative game.
- Sydney reviews storing controllers/followers as World components and the eventual NPC update iteration.
- D'Artagnan reviews collision clearance, ray queries, movement and platform/jump navigation.
- Gunveer reviews level grids, tile origin/size, dynamic obstacles and serialized patrol/configuration data.
- A behaviour tree is a possible later extension if concrete behaviours outgrow this state machine.
- Graphics, animations and a live NPC room demonstration need the relevant runtime adapters.

Tests cover original patrol compatibility, A* optimality across 256 obstacle layouts, unreachable/invalid endpoints, line-of-sight corners and symmetry, target selection, cooldowns, search/reacquisition, damage stun, reset, malformed inputs and an ECS/physics route-following scenario.

## Team workflow

Create or link a real Jira issue, agree estimates, and use its real key for the feature branch before publishing. Ask the owner and consumers to review the proposed API. Record actual test results and implementation understanding in the diary. Jira activity, teammate approval and a PR are pending.
