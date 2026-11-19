# Gunveer — Development Tools

Secondary responsibility: Software Developer/Architect.

## What is here

The neutral `LevelDocument` model supports validation, Tiled JSON object-layer import, and versioned native JSON save/load. It does not render tilemaps or create runtime entities.

- Public code: `engine/include/engine/tools.hpp`
- Implementation: `engine/src/tools.cpp`
- Tiled JSON importer: `engine/include/engine/tiled_importer.hpp`, `engine/src/tiled_importer.cpp`
- Example tests: `tests/tools_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^tools$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Level schema and importer** — Agree schema/version for two spawns, solid platforms, elemental hazards, linked plates/doors and two exits; import a selected Tiled JSON subset and reject broken object links. Proposed effort: 6 hours.

2. **Developer inspection panel** — Integrate approved Dear ImGui backend with Aryaman; show entities, timing and cache counts without owning those services. Proposed effort: 6 hours.

3. **Level round-trip and reload** — Save/load supported editable fields; test invalid data and ensure failed reload leaves the current level usable. Proposed effort: 6 hours.

The initial importer accepts finite orthogonal maps and top-level object layers. It maps rectangle objects, `class`/legacy `type`, spawn prefab properties, and `parent_id`/`linked_id` references into `LevelDocument`; malformed or unsupported object properties/shapes fail explicitly. Tile layers, tileset resolution/rendering, infinite maps, and runtime entity creation are not part of this importer increment.

## Native level persistence

`serialize_level_json` and `parse_level_json` use a native JSON document format identified by `"format": "cisc320.level-document"` and `schema_version: 1`. It is deliberately separate from Tiled JSON and from the proposed game-specific example under `assets/elemental_coop/levels/`. It round-trips the supported `LevelDocument` fields: name, pixel size, spawn IDs/prefabs/positions, and object IDs/types/prefabs/positions/sizes/parent and linked IDs. Unknown fields and unsupported versions are rejected.

`save_level_file` validates and serializes before writing a sibling temporary file and renaming it into place. The parent directory must already exist; level paths must be regular files and cannot be symlinks. `load_level_file` reads a fresh file value and parses/validates it. `reload_level` loads a complete candidate before replacing the active document; parse, validation, or file errors leave the active value unchanged. `LevelDocumentError` reports format/validation problems; `LevelFileError` reports path/read/write failures.

Tiled import remains a separate input path. Native persistence cannot preserve Tiled tile/GID data, layer metadata, object names, or unsupported custom properties because those are not represented in `LevelDocument`.

## Developer inspection backend (GUNVEER-02 Stage A)

`inspect_level_document` creates a value-only `LevelInspectionSnapshot` containing the schema version, name, bounds (origin zero through the document size), copied spawn and object definitions, and the same validation diagnostics returned by `validate`. It does not modify the source document and remains usable for invalid documents. `find_object` selects by stable level-object ID, not an ECS entity ID; it returns null for empty or missing IDs and for duplicate IDs that make selection ambiguous. Returned pointers refer to the snapshot and remain valid only while its object vector is not changed.

`InspectionMetrics` carries optional FPS, frame time in milliseconds, runtime entity count, and cached-resource count. The host supplies these values; disengaged optionals mean unavailable, while an explicitly supplied zero remains a measurement. The tools module does not query or own the ECS, renderer, or resource managers.

For graphical integration, the host can retain a `LevelDocument`, call `inspect_level_document` when it changes, display the snapshot's spawns/objects/diagnostics, and keep the selected object's stable ID for `find_object`. Use the existing `load_level_file`, `save_level_file`, and transactional `reload_level` APIs for a detached document inspection session; display their errors without creating or replacing runtime entities. The host may pass available metrics and leave all unavailable values unset.

The SDL3 Developer Tools screen is app-local under `apps/ui_prototype`. Stage B
adds a separate Level Document view to that existing screen using its current SDL
drawing and event helpers; no Dear ImGui or other GUI dependency is introduced.
The app links `engine_tools` and loads the bundled
`apps/ui_prototype/assets/inspector_sample.level.json` native-format example.

The host retains a detached `LevelDocument`, calls
`inspect_level_document` to refresh its `LevelInspectionSnapshot`, and uses
`find_object` with the selected stable level-object ID to show properties.
Document operations use `load_level_file`, `save_level_file`, and
`reload_level`; a failed load/reload leaves the last displayed document intact.
These operations do not create or replace ECS entities. The UI shows measured
SDL frame timing/FPS and `World::size()`; resource-cache metrics remain N/A
because this prototype has no cache-statistics interface. The live-player view
remains separate from level-document objects.

The UI prototype does not provide general ECS entity enumeration/component
snapshots; those require an agreed Sydney-owned ECS interface. Historical log
display requires an agreed Arvin-owned logger sink because `Logger` currently
writes only to a caller-provided stream. Stage B does not modify those systems,
the shared renderer, or the gameplay world. A future ImGui integration would
need owner approval and explicit input-capture integration with the existing SDL
event loop.

## Playable prototype campaign

The SDL prototype now has an app-local adapter in
`apps/ui_prototype/playable_level.hpp` that consumes the existing native
`LevelDocument` format and produces a `PlayableLevelDefinition`. It does not
change the shared schema or instantiate objects through a new engine API. The
adapter validates both the general document and the prototype's supported
gameplay subset before the active runtime is replaced. A failed parse or
semantic validation leaves the current level intact.

The native campaign data is in `apps/ui_prototype/assets/levels/`:

- `level_1_easy.level.json` reproduces the original Easy room geometry,
  spawns, hazards, lever, plate, door and exits. Easy retains its original
  same-player control behavior.
- `level_2_medium.level.json` adds separated routes, lava and water gaps,
  raised matching exits, and a distinct-operator cooperative gate.
- `level_3_hard.level.json` uses two sequential distinct-operator gates,
  lava/water/toxic hazards, and a final shared ascent to matching exits.

The adapter currently accepts 1280 × 720 documents with exactly one
`fire_player` and one `water_player` spawn. Spawns use feet coordinates and
must have a supported, in-bounds 28 × 46 collider with no hazard overlap.
Gameplay objects are limited to `platform/solid`, `hazard` with `lava`, `water`,
or `toxic`, `door` with `cooperative_gate` or
`cooperative_gate_distinct`, `lever/cooperative_lever`,
`pressure_plate/cooperative_plate`, and matching `fire_exit` and `water_exit`.
Each gate must have exactly one linked lever and plate; their `linked_id`s name
the door and their `parent_id`s name support platforms. Exits must also name
their supporting platform. Hard requires two ordered gates with controls on
their intended sides. Unsupported object types/prefabs, dangling links,
invalid supports, spawn collisions, and invalid sequential gate geometry are
rejected.

The prototype-local runtime uses the existing ECS and physics systems; it does
not modify `engine/` or `games/elemental_coop/`. Current movement parameters are
gravity 1000 px/s², jump velocity −630 px/s, horizontal speed 240 px/s, with
28 × 46 player colliders and solid platforms that block from below. The automated
walkthroughs simulate normal input frames and physics ticks to validate routes,
gate sequencing, hazard behavior, matching exits, and restart. Easy → Medium →
Hard progression and the final campaign-complete screen are checked by the SDL
smoke test. These are automated prototype checks, not a substitute for manual
playtesting on the target machine.

The walkthrough solution is documented in `apps/ui_prototype/README.md`.
Level authoring remains data-driven; any new entity type, shared gameplay rule,
physics tuning, renderer behavior, or UI/input behavior requires review by its
owning teammate before expanding beyond this prototype.

## Coordinate with

Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
