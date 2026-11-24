# Elemental Co-op game module — Blake

The game layer translates generic engine services into cooperative elemental puzzle rules. Public API: `include/elemental_coop/game.hpp`; implementation: `src/game.cpp`; CMake target: `elemental_coop_game`.

`Game` creates exactly two entities. Input slot 0 controls fire and slot 1 controls water. Held `move_x` selects horizontal velocity; one-shot `primary_pressed` means jump and only applies when grounded. A positive tick clears old exit contacts. Call hazard contacts first and refresh both matching-exit contacts after physics. Either character dying stops the shared round; both exit contacts win it. `restart()` resets both players, reusing their IDs.

`Game` is the flat-floor implementation used by the headless demo and tests. The SDL campaign loads native level files and applies its full puzzle and collision rules in [`DemoLevel`](../../apps/ui_prototype/demo_level.hpp). Both level adapters use the shared [host/client protocol](../../docs/NETWORKING.md).
