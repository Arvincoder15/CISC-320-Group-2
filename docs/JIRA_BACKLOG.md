# Proposed Jira backlog

These are draft tasks, not created Jira issues or evidence of completed student work. Estimates are initial hours, not commitments. Replace `SCRUM-<number>` with real Jira keys after review. Create/estimate tasks in Jira before starting the follow-up implementation, as required by the supplied course notes. Record the generated bootstrap separately and honestly.

Shared prerequisite: review `DECISIONS.md` and `ARCHITECTURE.md` together; assign an owner and reviewer to each interface.

## Sydney — Engine Architecture / ECS

### SYDNEY-01: Component storage and entity lifecycle

- Proposed assignee: Sydney
- Estimate: 6 hours
- Description / acceptance criteria: Add typed component add/get/remove operations, reject stale IDs, and test deletion cleanup.
- Dependencies / reviewers: Blake, D'Artagnan, Henry; Arvin for shared types and lifetime review
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### SYDNEY-02: Fixed-step loop and scenes

- Proposed assignee: Sydney
- Estimate: 6 hours
- Description / acceptance criteria: Run a fixed simulation step with a capped accumulator; test scene reset and input consumption across multiple ticks.
- Dependencies / reviewers: Blake, D'Artagnan, Henry; Arvin for shared types and lifetime review
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### SYDNEY-03: Events and integrated world

- Proposed assignee: Sydney
- Estimate: 5 hours
- Description / acceptance criteria: Agree event payloads with gameplay/physics/networking; demonstrate create, update, destroy without dangling references.
- Dependencies / reviewers: Blake, D'Artagnan, Henry; Arvin for shared types and lifetime review
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Aryaman — Rendering

### ARYAMAN-01: SDL3 window and renderer

- Proposed assignee: Aryaman
- Estimate: 6 hours
- Description / acceptance criteria: Add an approved pinned SDL3 dependency and RAII backend; show a window and handle initialization errors cleanly.
- Dependencies / reviewers: Arvin for texture handles; Sydney for transforms; Rohan for UI and SDL event ownership
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ARYAMAN-02: Cached sprite rendering

- Proposed assignee: Aryaman
- Estimate: 6 hours
- Description / acceptance criteria: Draw two sprites sharing one cached texture; validate missing-asset behavior and texture-before-renderer shutdown.
- Dependencies / reviewers: Arvin for texture handles; Sydney for transforms; Rohan for UI and SDL event ownership
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ARYAMAN-03: Camera and draw ordering

- Proposed assignee: Aryaman
- Estimate: 5 hours
- Description / acceptance criteria: Add camera transforms and layers; demonstrate game world to screen conversion with deterministic command tests.
- Dependencies / reviewers: Arvin for texture handles; Sydney for transforms; Rohan for UI and SDL event ownership
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## D'Artagnan — Physics & Collision

### DARTAGNAN-01: Collision components and layers

- Proposed assignee: D'Artagnan
- Estimate: 6 hours
- Description / acceptance criteria: Connect body/collider data to the shared ECS; test overlap, edge contact, disabled pairs, and entity removal.
- Dependencies / reviewers: Sydney for components; Blake for flap/collision semantics; Gunveer for collider data
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### DARTAGNAN-02: Collision events and triggers

- Proposed assignee: D'Artagnan
- Estimate: 6 hours
- Description / acceptance criteria: Emit contact events without applying game rules; gameplay consumes exactly one hit/trigger as appropriate.
- Dependencies / reviewers: Sydney for components; Blake for flap/collision semantics; Gunveer for collider data
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### DARTAGNAN-03: Movement integration and regression

- Proposed assignee: D'Artagnan
- Estimate: 4 hours
- Description / acceptance criteria: Agree fixed-step policy, gravity units and speed limits; test restart and document fast-moving collision limitations.
- Dependencies / reviewers: Sydney for components; Blake for flap/collision semantics; Gunveer for collider data
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Henry — Networking

### HENRY-01: Protocol and authority design

- Proposed assignee: Henry
- Estimate: 4 hours
- Description / acceptance criteria: Review two-player rules and authority with Blake; define versioned packet fields, bounds, IDs, and tick ordering.
- Dependencies / reviewers: Sydney for entity mapping; Blake for game state; Arvin for diagnostics; Rohan for lobby UI
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### HENRY-02: Two-process connection prototype

- Proposed assignee: Henry
- Estimate: 8 hours
- Description / acceptance criteria: Choose transport with team review; connect two local processes, handle disconnects, and document reproducible launch commands.
- Dependencies / reviewers: Sydney for entity mapping; Blake for game state; Arvin for diagnostics; Rohan for lobby UI
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### HENRY-03: Snapshot synchronization

- Proposed assignee: Henry
- Estimate: 8 hours
- Description / acceptance criteria: Synchronize agreed state between two clients; reject malformed/oversized/stale data and test disconnect/reconnect behavior.
- Dependencies / reviewers: Sydney for entity mapping; Blake for game state; Arvin for diagnostics; Rohan for lobby UI
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Rocco — Game AI

### ROCCO-01: Agent decision interface

- Proposed assignee: Rocco
- Estimate: 4 hours
- Description / acceptance criteria: Agree observation/action types with Blake; keep engine decisions independent of bird rules and test boundary conditions.
- Dependencies / reviewers: Blake for observation adapter; Sydney for entity access; D'Artagnan for collision queries
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ROCCO-02: Optional demonstration opponent

- Proposed assignee: Rocco
- Estimate: 6 hours
- Description / acceptance criteria: If scope is approved, map obstacle observations to actions with cooldown; demonstrate an agent using the same gameplay input path.
- Dependencies / reviewers: Blake for observation adapter; Sydney for entity access; D'Artagnan for collision queries
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ROCCO-03: AI evaluation and sprint coordination

- Proposed assignee: Rocco
- Estimate: 4 hours
- Description / acceptance criteria: Test decisions for invalid observations and repeatable scenarios; record agreed sprint scope and owners in Jira.
- Dependencies / reviewers: Blake for observation adapter; Sydney for entity access; D'Artagnan for collision queries
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Gunveer — Development Tools

### GUNVEER-01: Level schema and importer

- Proposed assignee: Gunveer
- Estimate: 6 hours
- Description / acceptance criteria: Agree schema/version with Arvin and Blake; implement selected Tiled JSON subset and reject unsupported/invalid data.
- Dependencies / reviewers: Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### GUNVEER-02: Developer inspection panel

- Proposed assignee: Gunveer
- Estimate: 6 hours
- Description / acceptance criteria: Integrate approved Dear ImGui backend with Aryaman; show entities, timing and cache counts without owning those services.
- Dependencies / reviewers: Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### GUNVEER-03: Level round-trip and reload

- Proposed assignee: Gunveer
- Estimate: 6 hours
- Description / acceptance criteria: Save/load supported editable fields; test invalid data and ensure failed reload leaves the current level usable.
- Dependencies / reviewers: Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Blake — Gameplay Systems

### BLAKE-01: Single-player round

- Proposed assignee: Blake
- Estimate: 8 hours
- Description / acceptance criteria: Add obstacle spawning, collision-driven loss and scoring in games/flappy; use engine components and deterministic tests.
- Dependencies / reviewers: All subsystem owners; especially Sydney, D'Artagnan, Henry, and Rohan
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### BLAKE-02: Restart and tuning

- Proposed assignee: Blake
- Estimate: 6 hours
- Description / acceptance criteria: Implement round states, complete reset and validated tuning data; repeated restart leaves no old entities or score.
- Dependencies / reviewers: All subsystem owners; especially Sydney, D'Artagnan, Henry, and Rohan
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### BLAKE-03: Multiplayer rules and game adapter

- Proposed assignee: Blake
- Estimate: 8 hours
- Description / acceptance criteria: Agree authority, score/win conditions and shared obstacle timing with Henry; integrate two clients using the engine protocol.
- Dependencies / reviewers: All subsystem owners; especially Sydney, D'Artagnan, Henry, and Rohan
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Rohan — User Interaction & UI

### ROHAN-01: SDL action mapping

- Proposed assignee: Rohan
- Estimate: 5 hours
- Description / acceptance criteria: Centralize SDL event polling; map keys to actions, handle quit/focus and test that one press is consumed once.
- Dependencies / reviewers: Aryaman for rendering; Blake for game states; Arvin for resources/settings; Gunveer for ImGui capture
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ROHAN-02: Menus and HUD

- Proposed assignee: Rohan
- Estimate: 6 hours
- Description / acceptance criteria: Implement title/pause/game-over UI and score display from game state; verify input capture with ImGui.
- Dependencies / reviewers: Aryaman for rendering; Blake for game states; Arvin for resources/settings; Gunveer for ImGui capture
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ROHAN-03: Audio and settings

- Proposed assignee: Rohan
- Estimate: 6 hours
- Description / acceptance criteria: Load approved sound assets through shared infrastructure; add volume validation and clean shutdown with the audio backend.
- Dependencies / reviewers: Aryaman for rendering; Blake for game states; Arvin for resources/settings; Gunveer for ImGui capture
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

## Arvin — Engine Infrastructure

### ARVIN-01: Framework and compiler decision

- Proposed assignee: Arvin
- Estimate: 5 hours
- Description / acceptance criteria: Compare GoogleTest/Catch2, agree C++ standard and dependency policy, migrate starter checks and validate Debug/Release CI.
- Dependencies / reviewers: Aryaman/Rohan for resources; Gunveer for schemas; Sydney for architecture; every owner for tests
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ARVIN-02: SDL asset cache and lifetime

- Proposed assignee: Arvin
- Estimate: 8 hours
- Description / acceptance criteria: Define typed texture/audio loaders with custom deleters, canonical keys and error results; prove shared loads and safe shutdown.
- Dependencies / reviewers: Aryaman/Rohan for resources; Gunveer for schemas; Sydney for architecture; every owner for tests
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.

### ARVIN-03: JSON configuration and diagnostics

- Proposed assignee: Arvin
- Estimate: 6 hours
- Description / acceptance criteria: Add approved JSON dependency, schema validation and explicit defaults; reject corrupt/missing required config and expose basic cache/timing data.
- Dependencies / reviewers: Aryaman/Rohan for resources; Gunveer for schemas; Sydney for architecture; every owner for tests
- Evidence: relevant automated or manual test results, linked PR, and Confluence diary entry.
