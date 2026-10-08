# Arvin — Engine Infrastructure

Secondary responsibility: Software Developer/Architect.

## What is here

Logger and injected typed ResourceCache work without SDL. JSON loading and an external test framework are not integrated.

- Public code: `engine/include/engine/infrastructure.hpp`
- Implementation: `engine/src/infrastructure.cpp`
- Example tests: `tests/infrastructure_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run your tests: `ctest --test-dir build -C Debug -R '^infrastructure$' --output-on-failure` (use double quotes in Windows shells).

## Why this subsystem exists

Your reusable subsystem should expose a small interface other teammates can test independently. Read [architecture and contracts](../ARCHITECTURE.md) before expanding it; Elemental co-op rules stay in `games/elemental_coop/`.

## First development tasks

1. **Framework and compiler decision** — Compare GoogleTest/Catch2, agree C++ standard and dependency policy, migrate starter checks and validate Debug/Release CI. Proposed effort: 5 hours.

2. **SDL asset cache and lifetime** — Define typed texture/audio loaders with custom deleters, canonical keys and error results; prove shared loads and safe shutdown. Proposed effort: 8 hours.

3. **JSON configuration and diagnostics** — Add approved JSON dependency, schema validation and explicit defaults; reject corrupt/missing required config and expose basic cache/timing data. Proposed effort: 6 hours.

## Coordinate with

Aryaman/Rohan for resources; Gunveer for schemas; Sydney for architecture; every owner for tests.

## Before opening a PR

Create a real Jira task, confirm any shared interface change with its owner, implement one reviewable increment, run your tests and the integration suite, and record results in your [diary](../templates/diary.md). Describe unfinished behavior clearly. The generated skeleton does not replace your substantive individual implementation.
