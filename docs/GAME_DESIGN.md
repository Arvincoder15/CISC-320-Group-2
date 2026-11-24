# Elemental Co-op game rules

Fire and Water work together through Easy, Medium, and Hard. The team clears a level when both players occupy their matching exits after opening its gates. Retry and Restart reset the active level.

Fire can cross lava and Water can cross water. The other element's liquid, toxic pools, and contact with acid slimes defeat either player and end the round for both. Slimes patrol and chase on the first two levels. On Hard, they pursue players across platforms.

| Level | Main puzzle |
| --- | --- |
| Easy | Toggle the lever and hold the pressure plate to unlock the gate, then reach both exits. |
| Medium | Use separate players for the lever and plate while navigating separate routes. |
| Hard | Open two gates in sequence while evading pursuing slimes. |

Local play uses two sets of controls on one keyboard. In multiplayer, Fire hosts and runs the authoritative game state. Water sends input and follows the host's level progression. See [controls](../README.md#play) and [level authoring](../apps/ui_prototype/README.md#playable-level-data-and-authoring).
