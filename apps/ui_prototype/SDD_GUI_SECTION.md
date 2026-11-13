# Graphical User Interface Prototype

The SDL3 prototype demonstrates the planned player-facing navigation and an initial
local-play flow. It uses the existing per-player input classes and engine physics, and
connects the settings screen to the engine configuration API. The screenshots for
this section should be captured from the running prototype. The earlier RAD
illustrations remain visual design references.

## Main Menu
The main menu provides Local Play, Multiplayer, Settings and Exit. Developer Tools
opens a separate inspection view. Buttons support mouse interaction and keyboard
navigation.

## Multiplayer Lobby
The lobby provides server-address entry and identifies the local player. Host,
Join and Start are disabled because network session integration is not included
in this prototype. No remote connection is simulated or reported as established.

## Gameplay
Local Play starts a playable reference room with two independently controlled
characters. Fire uses A/D to move and W to jump. Water uses Left/Right and Up.
The room includes stone platforms, lava, water, toxic liquid, an interactive lever,
a pressure plate and a locked central door. Fire survives lava and Water survives
water. Either player is defeated by toxic liquid or the other element's pool.

Either player can press E near the lever to toggle it. When the lever is active
and a player stands on the plate, the central door unlocks for the rest of the
round. Both matching upper exits then become active. The players must occupy
both exits together to complete the room. Restart resets the full puzzle.
The HUD shows controls, the current objective and Pause.

The room reuses the team's ECS and PhysicsSystem for platform collisions. Its
geometry and game-specific puzzle rules are implemented in the optional UI app.
Two stepping platforms and adjusted jump distances make the RAD composition
playable. The final Tiled-map/gameplay-module integration remains future work.

## Pause Menu
Pause provides Resume, Restart, Settings and Main Menu. Escape toggles local
pause. Losing window focus clears held input and pauses play, preventing movement
keys from remaining active when the player returns.

## Settings
Players can adjust master volume, sound effects, fullscreen mode and window size,
and view the controls. Apply updates the window and saves preferences using
`save_engine_settings`. Startup uses `load_user_settings`. The prototype selects
an isolated writable user folder with SDL_GetPrefPath. Invalid saved files are
preserved and defaults are used, with an error message. Audio controls affect a
short generated test sound. A complete music and sound-effects system is not yet
implemented. Back discards unapplied changes.

## Game Over
The game-over screen appears when either character touches a lethal hazard.
Retry restarts both players in the same test level. Main Menu returns to the title
screen.

## Level Complete
The completion screen appears when the puzzle is unlocked and both players occupy their matching exits.
Replay restarts the test level. Next Level is disabled because this prototype
contains one level. Main Menu returns to the title screen.

## Developer Tools
The developer view displays actual player positions, velocities, grounded state
and the live world entity count. The selected player can be changed and the
built-in demo level can be reloaded. Simulation pauses during inspection. This is
a minimal UI inspector, not the team's full level editor or Tiled importer.

## Navigation
Main Menu opens Local Play, Multiplayer Lobby, Settings or Developer Tools.
Gameplay opens Pause, Game Over or Level Complete. Pause can resume or restart
play, open Settings, or return to Main Menu. Settings returns to the screen that
opened it. Results allow retry/replay or return to Main Menu. Developer Tools
returns to its previous screen.

## Integration Boundary
The prototype owns one SDL event loop and a temporary renderer. The final app
must coordinate event routing and rendering with Aryaman, gameplay integration
with Blake, developer tooling with Gunveer, and settings storage with Arvin.
The prototype does not yet implement ImGui input capture or networked pause.

## Visual implementation and evidence
The prototype recreates the navy background, orange/blue buttons, elemental
characters and stone-platform composition using SDL drawing and a bundled font
atlas. It is an approximation of the RAD artwork, not a pixel-identical replica.
Capture screenshots from the running v2 executable for SDD implementation evidence.
Automated screen captures are staged UI states for layout review.

## Validation
All 12 existing engine test suites and the new reference-room suite passed on
Linux. The room suite completes the actual movement/jump route without teleporting,
checks the gate and elemental hazard rules, and verifies restart. An SDL smoke test
also checks keyboard routing, focus loss, navigation and settings persistence.
Physical Mac display/audio checks remain part of the manual validation.
