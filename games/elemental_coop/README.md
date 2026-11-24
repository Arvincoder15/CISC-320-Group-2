# Elemental Co-op game module — Blake

The game layer translates generic engine services into cooperative elemental puzzle rules. Public API: `include/elemental_coop/game.hpp`; implementation: `src/game.cpp`; CMake target: `elemental_coop_game`.

`Game` creates exactly two entities. Input slot 0 controls fire and slot 1 controls water. Held `move_x` selects horizontal velocity; one-shot `primary_pressed` means jump and only applies when grounded. A positive tick clears old exit contacts. Call hazard contacts first and refresh both matching-exit contacts after physics. Either character dying stops the shared round; both exit contacts win it. `restart()` resets both players, reusing their IDs.

`Game` is the flat-floor implementation used by the headless demo and tests. The SDL campaign loads native level files and applies its full puzzle and collision rules in [`DemoLevel`](../../apps/ui_prototype/demo_level.hpp). Both level adapters use the shared [host/client protocol](../../docs/NETWORKING.md).

## Shared gameplay rules

`gameplay_rules.hpp` defines elemental hazard immunity, the living-two-player exit condition, and `GateState`. Both the headless game and SDL campaign use the same hazard and exit policies. The campaign uses `GateState` for lever and plate logic; it remains responsible for detecting contacts, removing doors, updating navigation, and restarting the level. Ordinary levers toggle; distinct-operator levers latch and preserve their first operator. Gates stay open until restart. `update_plate` reports only the opening transition and refreshes plate occupancy even after the gate opens. Gameplay tests cover these policies without SDL.
