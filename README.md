# CISC 320 Group 2 — Multiplayer 2D Game Engine

Shared C++ starter for all nine developers. The reusable engine lives in `engine/`; the Flappy Bird-inspired demonstration lives in `games/flappy/`.

**Current state:** a working headless scaffold, not a graphical game or a multiplayer engine yet. SDL3, JSON, Dear ImGui, Tiled import, sockets, menus, audio, and full ECS storage remain owner tasks. Recording/loopback implementations are explicitly test doubles.

## Start here

1. [Build and run](docs/SETUP.md).
2. [Find your member handoff](docs/TEAM.md) — files, first tasks, tests and collaborators for every member.
3. Read [architecture and interface contracts](docs/ARCHITECTURE.md) and [decisions needing review](docs/DECISIONS.md).
4. Copy your proposed tasks from the [Jira backlog](docs/JIRA_BACKLOG.md), estimate them together, then begin implementation.
5. Use [CONTRIBUTING](CONTRIBUTING.md), the [shared AI knowledge base](docs/AI_KNOWLEDGE_BASE.md), and [Confluence templates](docs/CONFLUENCE.md).

## Quick start

Requires CMake 3.20+ and a compiler with C++20 support. No third-party library download is required for the starter.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Run `./build/engine_demo` on macOS/Linux, or `.\build\Debug\engine_demo.exe` with Visual Studio on Windows. It simulates 120 ticks and prints a summary; it does not open a window.

C++20 is a provisional bootstrap choice. The supplied team notes record C++26; the team must confirm its final standard and compiler matrix. The temporary CTest checks do not select GoogleTest or Catch2.

## Layout

```text
engine/include/engine/  Public reusable subsystem contracts
engine/src/            Eight engine subsystem implementations
games/flappy/         Blake's game-specific implementation (ninth ownership area)
apps/                  Executable composition and headless demo
tests/                Nine owner suites plus integration tests
assets/               Example configuration and future licensed assets
docs/members/         Individual handoffs for all nine members
docs/templates/       Jira, diary, RAD, SDD, meeting and decision templates
.github/               Proposed build CI and pull request template
```

See [milestones](docs/ROADMAP.md) for a proposed integration sequence. Course dates and requirements in these documents come from the supplied conversation notes and need verification against the course site.
