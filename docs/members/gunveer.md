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

## Coordinate with

Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
