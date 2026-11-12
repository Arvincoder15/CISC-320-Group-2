# Gunveer — Development Tools

Secondary responsibility: Software Developer/Architect.

## What is here

The neutral `LevelDocument` model supports validation and import of a documented Tiled JSON object-layer subset. It does not render tilemaps or create runtime entities.

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

## Coordinate with

Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
