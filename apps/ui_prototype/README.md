# Elemental Co-op SDL application

This executable runs the three-level campaign, local and online multiplayer, settings, and the developer inspector. See the [main README](../../README.md#build-and-run) for build and launch commands.

## What works

See [Remote multiplayer](../../docs/ROOM_RELAY.md) for room codes, a two-window test, and relay deployment.

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
- Menu selections, Fire and Water jumps, levers, opening doors, losing and
  finishing a level have distinct sounds. Each peer plays effects locally from
  observed gameplay; no audio data travels over the network. **Preview Sound**
  plays Water's jump using the settings on screen before Apply. Zero volume or
  switching effects off silences playback. A missing clip leaves the game playable
  and prints the affected file name. No extra audio library is needed.
- An original background track loops through the menus and gameplay on a separate
  SDL stream. Master volume controls both the music and effects. The effects
  toggle mutes only the cues. Press **M** or use the bottom-right music icon
  to toggle background music for the current session. Regenerate the music with
  `python3 apps/ui_prototype/tools/make_music.py` from the repo root.
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
  the font atlas and the bundled WAV files when audio is available), not SDL textures
  or audio streams. Metrics
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
subfolder. The packaged assets remain read-only during normal play.
Corrupt files are not silently overwritten. Back up/rename the rejected file
explicitly if you want to reset it, then restart or Apply again.

## Bundled assets

The UI uses a DejaVu Sans Bold atlas. Its redistribution license is in
[`assets/FONT-LICENSE.txt`](assets/FONT-LICENSE.txt). The SDL renderer uses the
[shared rendering assets](../../assets/rendering/README.md) for platform tiles
and player sprites.

The seven original sound effects are in `assets/audio/`. To regenerate them,
run `python3 apps/ui_prototype/tools/make_sounds.py` from the repository root.
The game reads the bundled files directly.

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

See [testing](../../docs/TESTING.md#manual-checks) for the desktop and multiplayer checks.

## Files and verification helpers

`main.cpp` owns rendering/events/settings and campaign progression.
`playable_level.hpp` validates and maps native JSON into app-local playable
definitions. `demo_level.hpp` owns the level-local runtime and puzzle rules.
`demo_walkthrough.hpp` is the deterministic physics walkthrough shared by the
test and `--smoke-test`. `level_test.cpp` runs adapter, rule, and route checks.
`font_metrics.hpp` and
`assets/font.bmp` hold the text atlas.
