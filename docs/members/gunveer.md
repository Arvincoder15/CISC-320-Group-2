# Gunveer — Development Tools

Secondary responsibility: Software Developer/Architect.

## What is here

LevelDocument validation is a neutral data model. It does not load Tiled or JSON.

- Public code: `engine/include/engine/tools.hpp`
- Implementation: `engine/src/tools.cpp`
- Example tests: `tests/tools_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^tools$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Flappy rules stay in `games/flappy/`.

## First development tasks

1. **Level schema and importer** — Agree schema/version with Arvin and Blake; implement selected Tiled JSON subset and reject unsupported/invalid data. Proposed effort: 6 hours.

2. **Developer inspection panel** — Integrate approved Dear ImGui backend with Aryaman; show entities, timing and cache counts without owning those services. Proposed effort: 6 hours.

3. **Level round-trip and reload** — Save/load supported editable fields; test invalid data and ensure failed reload leaves the current level usable. Proposed effort: 6 hours.

## Coordinate with

Sydney for entities; Arvin for JSON/file utilities; Aryaman/Rohan for SDL and ImGui input handling.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
