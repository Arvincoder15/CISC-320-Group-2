# Elemental co-op game module — Blake

The game layer translates generic engine services into cooperative elemental puzzle rules. Public API: `include/elemental_coop/game.hpp`; implementation: `src/game.cpp`; CMake target: `elemental_coop_game`.

`Game` creates exactly two entities. Input slot 0 controls fire and slot 1 controls water. Held `move_x` selects horizontal velocity; one-shot `primary_pressed` means jump and only applies when grounded. A positive tick clears old exit contacts. Call hazard contacts first and refresh both matching-exit contacts after physics. Either character dying stops the shared round; both exit contacts win it. `restart()` resets both players, reusing their IDs.

Current limitations: positions represent feet; only a flat floor is enforced; there are no character-to-character, wall, ceiling or tile collisions. Contacts are manually supplied integration seams. No actual level file is parsed, and no switch/door, graphical UI, keyboard polling or socket transport exists. See [design](../../docs/GAME_DESIGN.md) and [Blake's handoff](../../docs/members/blake.md).

Keep all elemental identities, hazard rules, puzzle links, collectibles and level progression here. Engine physics and networking should accept generic components/state without depending on fire/water rules.
