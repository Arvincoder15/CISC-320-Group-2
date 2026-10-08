# Decision register

| Topic | State | Proposed next action / owner |
|---|---|---|
| Reusable engine, C++, nine ownership areas, Flappy-inspired demo | Confirmed in supplied notes | Preserve engine/game separation |
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
| Two-player competitive rules | Suggested, not approved | Team agrees win/loss, restart and shared obstacles |
| AI opponent | Optional proposal | Rocco/Blake agree a useful, bounded demo feature |
| Repository host/course access | Needs confirmation from supplied notes | Sydney/Henry confirm with TA |
| Integration timeline | Proposed | Rocco aligns sprint capacity and course dates |

For each accepted change, copy `templates/decision.md`, add reviewers/date and update affected code, tests, Jira and Confluence. Preserve the distinction between a proposal and an approved team decision.
