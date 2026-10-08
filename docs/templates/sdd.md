# System Design Document — draft template

1. Architectural overview and engine/game dependency diagram.
2. Subsystems, classes, attributes, methods and ownership.
3. UML class diagrams based on actual implementation.
4. Sequence diagrams: startup/shutdown, two-player input-to-frame, resource load, multiplayer update.
5. ECS, event, resource, level and protocol contracts.
6. Error handling, file schemas and version compatibility.
7. GUI screenshots and tools.
8. Individual programming assignments and testing responsibilities.
9. Sprint milestones, timeline, repository evidence and real burndown charts.
10. Test strategy/results, limitations and changes since RAD.

Link diagrams to real classes. Record pending design decisions explicitly; replace proposals with approved details after review.

## Current demonstration details to include

Document `Game`, `Settings`, `PlayerState`, `PlayerInputs`, `InputBuffer`, `World`, `Body`, `Renderer` and `SnapshotTransport` from the actual code. Add planned collider/trigger, plate/door, level importer and network session classes only with an explicit proposed status.

Useful sequences: two independent inputs to the shared room; pressure plate contact to door state; lethal hazard to shared restart; both exit contacts to completion; host processing inputs and publishing a full cooperative snapshot. Include actual GUI screenshots once constructed, not headless output presented as GUI evidence.
