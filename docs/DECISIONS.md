# Decision register

| Topic | State | Proposed next action / owner |
|---|---|---|
| Reusable engine, C++, nine ownership areas, cooperative elemental puzzle-platformer demo | Confirmed by user correction on Oct 8, 2026 | Preserve engine/game separation |
| Jira and Confluence | Required in supplied notes | All members maintain evidence |
| C++ standard | Notes say C++26; starter uses provisional C++20 | Sydney/Henry/Arvin verify every machine and record team choice |
| Compiler matrix | Pending | Record compiler, SDK and CMake versions for all members; run CI |
| SDL3, JSON, ImGui, Tiled | Intended stack; exact versions not selected | Owners propose pinned versions and licenses |
| Dependency acquisition | Pending | Choose one reproducible method before external integration |
| GoogleTest vs Catch2 | Pending | Arvin presents choice; CTest runner is temporary |
| Full ECS and events | Pending | Sydney reviews starter contracts and component design |
| Resource lifetime | Proposed in ARCHITECTURE.md | Arvin, Aryaman and Rohan review before SDL assets |
| JSON and level schemas | Example only | Arvin/Gunveer define schema and invalid-input behavior |
| Network transport and authority | Pending | Henry and Blake prototype and document |
| Cooperative multiplayer | Confirmed direction; detailed rules proposed | Two human roles; review shared loss, matching exits, host authority and restart |
| Optional patrol hazards/NPCs | Proposed | Rocco/Blake agree a bounded state-machine demo; both characters remain human-controlled |
| Repository host/course access | Needs confirmation from supplied notes | Sydney/Henry confirm with TA |
| Integration timeline | Proposed | Rocco aligns sprint capacity and course dates |

For each accepted change, copy `templates/decision.md`, add reviewers/date and update affected code, tests, Jira and Confluence. Preserve the distinction between a proposal and an approved team decision.
