# Elemental Co-op: reference-room UI prototype (v2)

This optional C++20 / SDL3 executable is based on Rohan's branch at `6532d4e`.
It was also checked against main at `a17953f` (Gunveer's tools merge).
It adds a graphical application without changing the shared engine interfaces or
root CMake configuration. All new source is under `apps/ui_prototype/`.
It is a prototype for review, not the completed game.

## Build on macOS

From the repository root:

```sh
cmake -S apps/ui_prototype -B build-ui -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
cmake --build build-ui --parallel
ctest --test-dir build-ui --output-on-failure
./build-ui/ui_prototype
```

Requires SDL 3.2 or newer, already installed on Rohan's Mac. No SDL_ttf or SDL_image installation is needed. Keep the generated assets folder alongside the executable. CMake copies it automatically.

Requires the repository including the physics/settings/tools merges through main `a17953f`. Tested here with SDL 3.2.28 and GCC on Linux.
The Mac build and real keyboard/audio checks must be performed on your Mac.
No automatic dependency downloads occur during CMake configuration.
If Homebrew cannot install SDL on your macOS version, build SDL 3.2.28 from
its official source into a user-owned folder and supply that prefix to CMake.

## What works

- Main menu: Local Play, Multiplayer, Settings, Exit, Developer Tools.
- Real SDL keyboard events map to the existing per-player `PlayerInput` classes.
- Fire: A/D to move, W to jump. Water: Left/Right to move, Up to jump.
- Movement releases are handled. Key-repeat events do not retrigger jumping.
- Losing window focus clears held input and pauses local play.
- Escape pauses/resumes. Menu buttons support mouse clicks and Tab/Up/Down/Enter.
- The app-local `DemoLevel` uses the team's `engine::World`, `PhysicsSystem`,
  `Body`, `Collider` and `ContactState` for an actual platform room.
- The room follows the reference composition: Fire at lower left, Water at lower
  right, three pools along the bottom, raised central door, upper matching exits.
  Two extra stepping platforms and tuned jumps make the illustrated layout playable.
- Fire survives lava, Water survives water, and toxic liquid defeats either player.
  Touching the other element's liquid defeats the team. Both can jump over pools.
- E flips the lever when either player is nearby. With the lever on, standing on
  the pressure plate unlocks the centre door permanently for that round. Its solid
  collider is removed. Both exits activate only after the door unlocks.
- Both players must occupy their matching upper exits together to complete the room.
  Restart resets players, lever, plate, door and completion state.
- Pause: Resume, Restart, Settings, Main Menu.
- Settings: volume, effects toggle, fullscreen, two window sizes, controls display.
  Apply updates the window and saves via Arvin's API. Back discards pending edits.
- Audio settings control a generated short test tone. No external sound assets required.
- Startup loads settings using `load_user_settings`. Invalid files are preserved,
  defaults are used, and the terminal/UI reports the failure.
- Apply uses `save_engine_settings` and creates the parent directory first.
  A failed save rolls the requested window state back and retains prior settings.
- Developer Tools shows actual player positions, velocities, grounded state and
  entity count. Switch selection or reload the built-in level. Simulation is paused here.
- F12 saves the current rendered frame as BMP, logging its location in the terminal.

## Storage

The app uses `SDL_GetPrefPath("CISC320Group2", "ElementalCoopPrototype")`.
SDL selects and creates a platform-appropriate per-user writable directory.
The full settings path is printed at launch. Screenshots are in its `screenshots/`
subfolder. This is an isolated prototype location for team review, not a final
team-wide storage policy. It never writes configuration into packaged assets.
Corrupt files are not silently overwritten. Back up/rename the rejected file
explicitly if you want to reset it, then restart or Apply again.

## Scope and integration handoff

- Multiplayer address entry is a UI preview. Host, Join and Start are disabled.
  No connection status is fabricated and no real network session is created.
- One level only. Next Level is disabled on completion.
- Room rules and geometry live in `demo_level.hpp` within this app. They do not
  replace Blake's shared gameplay module. This is a temporary playable reference
  room, not the final Tiled-level integration. Review the puzzle rule with the team.
- Artwork is drawn with SDL primitives, with a bundled DejaVu Sans Bold glyph atlas
  for smooth text. Its redistribution license is in `assets/FONT-LICENSE.txt`.
  No new runtime library is needed. Original RAD images are design references,
  not screenshots of this executable. The recreation is not pixel-identical.
- Developer Tools is a minimal live inspector, not Gunveer's full editor. File
  loading, entity editing and saving level JSON remain his integration work.
- This executable owns one SDL event loop and renderer. Coordinate with Aryaman
  before moving it into the final app. Future ImGui input capture must be respected.
- Audio is a short generated tone only, not a finished music or effects manager.
- No root engine APIs or another member's files are modified.
- Track expanded UI scope in Jira and request rendering/gameplay/infrastructure
  review before merging. Do not describe this as complete SCRUM-10 integration
  into the final graphical application.

## Verification

Automated smoke test (uses an isolated temporary settings path):

```sh
./build-ui/ui_prototype --smoke-test
```

On headless Linux, prefix with `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`.
The smoke test checks menu navigation, independent player keyboard movement/jump,
focus clearing, settings save/load, hazard-to-Game-Over routing and a reachable win.
`test_ui_level` additionally checks all six elemental hazard combinations, remote
lever rejection, cooperative unlocking, one-exit insufficiency and restart. Its
walkthrough uses normal input frames and physics, with no teleports or forced wins.
It does not verify sound through a physical speaker or OS fullscreen behaviour.

Capture all eight rendered screen layouts at 1920 x 1080:

```sh
./build-ui/ui_prototype --smoke-test "$HOME/Desktop/Elemental-UI-Captures"
```

These are programmatically staged running UI states for layout review. For SDD
interaction evidence, also take screenshots during your manual walkthrough.
On Mac use Command+Shift+4 then Space to capture the application window.

Manual checks before SDD submission:
1. Open Local Play. Move both players at once. Release one player's movement key.
2. Jump, keep holding the jump key through landing, and confirm no second jump.
3. Hold movement, switch to another app, return and Resume. Confirm no stuck key.
4. Exercise Pause, Restart, Settings and Main Menu, including Escape and mouse.
5. Apply fullscreen/window size and volume. Test audio with effects on/off.
6. Close and reopen the application. Confirm settings persist.
7. Test the elemental pools and Retry. Complete the lever/plate puzzle and reach both exits.
8. Open F3 and confirm live values match the selected player. Reload and return.
9. Check the lobby and Next Level clearly indicate unfinished functionality.

## A playable walkthrough

Fire uses A/D/W. Water uses Left/Right/Up. Release the jump key between jumps.

1. Move Fire toward the left lava pool and jump right to the switch island.
2. Stand beside the orange lever and press E once. Its handle turns green.
3. Move Water left and jump over the toxic pool onto the pressure-plate island.
4. Stand on the cyan plate. With the lever on, the centre door opens and stays open.
5. Fire jumps left onto the small stepping platform, right onto the middle-left
   platform, then left onto the upper Fire-exit platform.
6. Water jumps right onto the small stepping platform, left onto the middle-right
   platform, then right onto the upper Water-exit platform.
7. Park each character inside the matching exit. Both must be there together.

The central platforms can also be traversed once the door opens. Exit activation
is gated by the puzzle, so jumping around the door does not bypass unlocking it.

## Files and verification helpers

`main.cpp` owns rendering/events/settings. `demo_level.hpp` owns the reference room.
`demo_walkthrough.hpp` is the deterministic physics walkthrough shared by the test
and `--smoke-test`. `level_test.cpp` runs the room checks. `font_metrics.hpp` and
`assets/font.bmp` hold the text atlas. `SDD_GUI_SECTION.md` describes this version.

Version 2 verification: 13 CTest suites passed on Linux with SDL 3.2.28, including
all 12 existing engine suites. The SDL smoke test passed with dummy video/audio.
The rendered layouts were inspected. Real Mac display, fullscreen, sound and
physical-keyboard rollover still need the manual checks above.
