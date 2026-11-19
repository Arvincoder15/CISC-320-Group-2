# Elemental Co-op: SDL3 UI prototype

This optional C++20 / SDL3 executable is based on Rohan's branch at `6532d4e`.
It was also checked against main at `a17953f` (Gunveer's tools merge).
It adds a graphical application without changing the shared engine interfaces or
root CMake configuration. The app-local campaign has three native JSON level
definitions. It remains a prototype, not the completed game.

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
  `Body`, `Collider` and `ContactState` for playable platform levels.
- Easy preserves the original reference-room geometry and gameplay: Fire at lower left, Water at lower
  right, three pools along the bottom, raised central door, upper matching exits.
- Fire survives lava, Water survives water, and toxic liquid defeats either player.
  Touching the other element's liquid defeats the team. Both can jump over pools.
- Easy retains its original lever-toggle and pressure-plate puzzle. Medium and
  Hard require distinct players to operate each gate's lever and plate; the lever
  latches on, and successfully opened gates remain open until restart. Hard has
  two sequential gates, with Gate A blocking access to Gate B's controls.
- Both players must occupy their matching exits together, with all gates open, to
  complete a level. Next Level advances Easy → Medium → Hard. Hard completion
  shows the final campaign state. Restart and Retry reset only the active level.
- Pause: Resume, Restart, Settings, Main Menu.
- Settings: volume, effects toggle, fullscreen, two window sizes, controls display.
  Apply updates the window and saves via Arvin's API. Back discards pending edits.
- Audio settings control a generated short test tone. No external sound assets required.
- Startup loads settings using `load_user_settings`. Invalid files are preserved,
  defaults are used, and the terminal/UI reports the failure.
- Apply uses `save_engine_settings` and creates the parent directory first.
  A failed save rolls the requested window state back and retains prior settings.
- Developer Tools preserves the live-player inspector and adds a separate Level
  Document view. It shows the native document name, bounds, spawn/object counts,
  validation diagnostics, a paged object list and selected-object properties.
  Selection uses stable document object IDs; document objects are not ECS entities.
- The Level Document view loads, saves and safely reloads native
  `cisc320.level-document` JSON through Gunveer's Development Tools API. Enter a
  file path and use Ctrl+A (Command+A on macOS) to replace it. The bundled
  `assets/inspector_sample.level.json` demonstrates spawn definitions and linked
  door/exit/pressure-plate objects. Saving over the bundled sample is disabled.
- FPS and frame time are measured from SDL frame intervals, the live world entity
  count comes from the existing `World`, and cached file resources come from
  `FileResourceManager::size()`. The latter counts cached file data (currently
  the font atlas), not SDL textures, audio resources, or all game assets. Metrics
  remain optional and display as N/A when the host does not supply a value.
  Level-document file operations use a detached `LevelDocument`; they do not
  load or mutate the playable ECS world.
- F12 saves the current rendered frame as BMP, logging its location in the terminal.

## Acid enemies

The campaign contains one acid slime on Easy, three on Medium, and five on Hard.
They use the same bright green as toxic pools, animate as they move, and cause
Game Over when their body overlaps either player. Jump over them; neither player
has elemental immunity to an enemy. Enemies do not operate levers or plates.
Pause freezes their movement and animation; Retry, Restart, and level transitions
recreate their positions and AI state.

Each JSON level contains named `acid_enemy` entries in `spawns`, with feet-position
coordinates. Spawn validation checks platform support, clearance, unique IDs,
no enemy overlap, and at least 80 pixels of separation from player spawns.
A slime must also start on a different floor from both players. Platforms
that touch at the same height count as one floor. Gaps or height changes
separate floors. In Medium, both players start on raised ledges above the
slimes' patrol routes.

On Easy and Medium, each slime patrols the full walkable span of its platform
at 92 pixels/s. It
chases the nearest grounded player on that connected floor at any distance,
regardless of which way either character faces. Gaps, hazards, and closed
doors interrupt the route. When a player leaves the floor, the slime briefly
searches the last reachable position before resuming patrol.

On Hard, each slime pursues a player from the start. The five slimes split into
groups of two and three, using the closest overall assignments. They walk at
92 pixels/s and can jump to reachable platforms, including narrow ledges and
the platforms that carry levers and pressure plates. They use the same jump
height as players and a lower horizontal speed. Hunters can walk immediately,
with a short starting delay before their first jump and a recovery after each
landing. They pursue a jumping player's last grounded position until the
player lands. At a closed gate, slimes move as close as they can and take
advantage of new routes when the gate opens. This behavior is
enabled for playable levels whose names end in ` - Hard`.

`test_ui_enemies` covers counts, spawn validation, patrol, pursuit, both players'
contact deaths, jumping over enemies, obstacles, and restart. The three campaign
walkthroughs run with enemies enabled, and the UI smoke test checks enemy pause and
Game Over routing. No additional external libraries or art downloads are required.

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
- Developer Tools provides a live-player inspector and a detached Level Document
  inspector. It does not edit live ECS entities or instantiate level objects.
  Native document persistence is not Tiled JSON import; the existing Tiled
  importer remains a separate engine API. This is not a full visual level editor.
- This executable owns one SDL event loop and renderer. Coordinate with Aryaman
  before moving it into the final app. Future ImGui input capture must be respected.
- Audio is a short generated tone only, not a finished music or effects manager.
- No root engine APIs or another member's files are modified.
- Track expanded UI scope in Jira and request rendering/gameplay/infrastructure
  review before merging. Do not describe this as complete SCRUM-10 integration
  into the final graphical application.

## Playable level data and authoring

The campaign definitions are `assets/levels/level_1_easy.level.json`,
`level_2_medium.level.json`, and `level_3_hard.level.json`. They use the existing
`cisc320.level-document` JSON format and are converted by the app-local
`playable_level.hpp` adapter. This is a deliberately small gameplay subset of
`LevelDocument`, not a new shared schema or an editor feature.

Playable documents must be 1280 × 720 and contain exactly one `fire_player` and
one `water_player` spawn. Spawn positions are feet coordinates and must place the
complete 28 × 46 player collider on a supporting platform, inside the world, and
outside hazards. Supported objects are solid platforms; lava, water, and toxic
hazards; one or two linked cooperative gates; one linked lever and pressure
plate per gate; and one matching exit per player. Each control's `linked_id`
names its door, and its `parent_id` names its support platform. Exit
`parent_id`s likewise identify their support. Unsupported types/prefabs,
malformed/dangling links, invalid supports, and invalid gate arrangements are
rejected before the active level is replaced.

The app's current physics uses gravity 1000 px/s², jump velocity −630 px/s,
horizontal speed 240 px/s, and solid platforms that block from above and below.
When authoring a raised route, leave a side approach for each jump; a player
cannot jump upward through a platform. Fire is immune only to lava and Water
only to water; toxic and the other element's liquid are lethal. Run the physical
walkthrough test after changing layout: it uses normal input frames and physics,
not teleporting.

Puzzle solutions:

- **Easy:** Fire activates the lever; Water stands on the plate; then both use
  their matching upper routes and occupy the exits together. This keeps the
  original single-gate interaction rule.
- **Medium:** Fire reaches and latches the lever; Water crosses to the plate
  island and holds the plate. After the gate opens, Fire and Water take their
  separate raised routes to their matching exits. Attempting both controls with
  the same player does not open the gate.
- **Hard:** Fire reaches Gate A's lever and stays on its plate; Water activates
  Gate A's plate. After Gate A opens, Fire uses the elevated route to latch Gate
  B's lever and holds its plate; Water activates Gate B's plate. Both gates stay
  open, allowing the two players to climb the final ascent and reach the exits.

## Verification

Automated smoke test (uses an isolated temporary settings path):

```sh
./build-ui/ui_prototype --smoke-test
```

On headless Linux, prefix with `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy`.
The smoke test checks menu navigation, independent player keyboard movement/jump,
focus clearing, settings save/load, hazard-to-Game-Over routing, physical wins
and Easy → Medium → Hard progression through the UI's Next Level action, final
campaign completion, and the level inspector. It also checks stable-ID selection,
editable-path loading, native save/reload, failed-reload preservation, and that
document operations leave live ECS entities and player positions unchanged.
`test_ui_level` checks native JSON and semantic validation, invalid-load
preservation, safe spawns, elemental hazards, cooperative controls, gate
collisions and latching, matching exits, restart, and physical input/physics
walkthroughs for all three levels. The walkthroughs do not teleport or force wins.
These automated checks do not substitute for manual gameplay, speaker/audio, or
OS fullscreen testing.

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
7. Test each level's elemental pools and Retry. Complete the lever/plate puzzle
   with two players, advance through all three levels, and confirm the final
   completion screen. Verify that restarting Medium or Hard resets only that
   level's gates, controls, spawns and win state.
8. Open F3 and confirm live values match the selected player. Switch to Level
   Document, select an object, inspect its relationships, and load/save/reload a
   copy of the bundled sample. Try malformed JSON and confirm the last valid
   document remains visible. Return to gameplay and verify it is unchanged.
9. Check the lobby clearly indicates that multiplayer is unfinished.

## Files and verification helpers

`main.cpp` owns rendering/events/settings and campaign progression.
`playable_level.hpp` validates and maps native JSON into app-local playable
definitions. `demo_level.hpp` owns the level-local runtime and puzzle rules.
`demo_walkthrough.hpp` is the deterministic physics walkthrough shared by the
test and `--smoke-test`. `level_test.cpp` runs adapter, rule, and route checks.
`font_metrics.hpp` and
`assets/font.bmp` hold the text atlas. `SDD_GUI_SECTION.md` describes this version.

Automated verification and manual playtesting are reported separately; the
headless SDL smoke test does not verify real display, fullscreen, audio output,
or physical keyboard rollover. See the latest implementation report for the
exact test commands and results.
