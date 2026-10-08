# Proposed milestones

Dates come from the supplied course summary, not independent verification. Confirm them against the course site. This is a planning draft for Rocco and the team.

| Period | Integration outcome | Evidence |
|---|---|---|
| Oct 8–14 | Everyone builds starter; contracts/standard/dependencies reviewed; SDL window and two-process network spikes started | Build matrix, decision records, Jira estimates |
| Oct 15–23 | Two independent inputs → movement/jump → two visible characters; floors/walls and networking connection proof | Demo and integration tests, requirements draft |
| Oct 24 | RAD deadline recorded in notes | Reviewed functional requirements and GUI sketches in Confluence |
| Oct 25–Nov 6 | Playable local cooperative room with hazards, linked plate/door, both exits and shared restart; early state synchronization | UML/sequence diagrams, tests, sprint evidence |
| Nov 7 | SDD deadline recorded in notes | Architecture, assignments, screenshots and testing responsibilities |
| Nov 8–20 | Networked cooperative room with synchronized puzzle state, disconnect behavior, menus/audio/tools and optional patrol hazard | Two-process demo, regression tests, updated docs |
| Nov 21–27 | Feature freeze, bug fixes, clean-machine build, presentation rehearsal | Repeatable demo script, test report, contribution evidence |
| Nov 28 | Final presentation date recorded in notes | Approximately ten-minute demo with fallback recording |

Integrate a small vertical slice each sprint. Prototype networking early while local cooperative gameplay grows. Keep prediction, matchmaking, large-scale ECS optimizations, complex save states, particles and advanced profiling outside the critical path unless the core loop is stable.
