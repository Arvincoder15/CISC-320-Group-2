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

The SDL3 Developer Tools screen is app-local under `apps/ui_prototype`; this backend intentionally does not modify that UI. That prototype currently has no Dear ImGui integration, and the shared renderer is only a draw-command interface. A future panel must use an owner-approved UI/event/backend integration and add `engine_tools` to the prototype's link dependencies if that target consumes the API. General live entity enumeration/component snapshots require an agreed Sydney-owned ECS interface; historical log display requires an agreed Arvin-owned logger sink because `Logger` currently writes only to a caller-provided stream. No UI, ECS, renderer, logger, resource-manager, dependency, or CMake changes are part of Stage A.

## Coordinate with

Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
