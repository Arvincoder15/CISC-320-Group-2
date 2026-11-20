# Networking (Henry)

Two players on two computers on the same local network play one co-op level. The player who hosts runs the
game and controls **fire** (slot 0); the player who joins controls **water** (slot 1). Design decisions and the
reasons behind them are in [DECISIONS.md](DECISIONS.md), ADRs NET-1 to NET-5.

**Status:** connection, protocol, host/client sync of the playable game (players, levers, plates, doors, acid
enemies, campaign level), disconnect detection and a two-process demo work and are tested. Not yet done: the
lobby buttons and game loop in `apps/ui_prototype/main.cpp` (Rohan, see [For Rohan](#for-rohan-wiring-the-lobby)).

## The pieces

| What | Where | Who uses it |
|---|---|---|
| `engine::net::Session`: connect and move bytes for one other player | `engine/include/engine/networking.hpp` | Lobby (Rohan), game loop |
| ENet implementation, `make_enet_session()` | `engine/src/enet_session.cpp` | Real play |
| In-memory pair with delay/loss, `make_loopback_pair()` | `engine/src/networking.cpp` | Tests only |
| Co-op messages, `NetLevel`, `CoopHost`, `CoopClient` | `games/elemental_coop/include/elemental_coop/net.hpp` | Drivers for any level |
| `OnlinePlay` (lobby + game loop helper), `DemoNetLevel` | `apps/ui_prototype/net_play.hpp` | The playable game (Rohan) |
| `DemoLevel::apply_network_state()` | `apps/ui_prototype/demo_level.hpp` | Joiner's copy of the level |

ENet v1.3.18 is downloaded by CMake on the first configure, so that first configure needs internet. Nothing
outside `engine/src/enet_session.cpp` includes ENet.

## For Rohan: wiring the lobby

Everything the UI needs is one object, `prototype::OnlinePlay` in `apps/ui_prototype/net_play.hpp`. It owns
the connection, runs the host's level or keeps the joiner's level in sync, and gives you status text. You never
touch the protocol. Its whole flow (lobby, Level 1 lever and plate, pause, level switch, disconnect) is tested in
`apps/ui_prototype/net_test.cpp`.

**1. Member** in `App`, declared after `game_` (it keeps references to `game_`, `campaign_`, `campaign_index_`):

```cpp
#include "net_play.hpp"
prototype::OnlinePlay online_{game_, campaign_, campaign_index_};
```

**2. Lobby buttons**

| Button / key | Call |
|---|---|
| HOST SESSION | `online_.host();` (port 7777) |
| JOIN SESSION | `online_.join(address_);` |
| START GAME (host only, enable when `online_.connected()`) | `start_campaign(); online_.start();` |
| BACK / MAIN MENU | `online_.leave();` |
| Status line | `text(..., online_.status())` |

**3. Game loop.** While `online_.active()`, call `online_.update(...)` **every frame on every screen** (it also
does the handshake in the lobby), instead of `game_.tick(...)`:

```cpp
if (online_.active()) {
    online_.update({inputs_[0].consume(), inputs_[1].consume()}, dt); // both key sets move this player's character
    if (online_.connection_lost()) { notice_ = online_.status(); online_.leave(); go(Screen::menu); }
    else if (screen_ == Screen::lobby && online_.playing()) go(Screen::play);
} else if (screen_ == Screen::play) {
    game_.tick({inputs_[0].consume(), inputs_[1].consume()}, dt); // local play, unchanged
}
// then keep the existing lost/won checks on game_.state(); on the joiner it comes from the host
```

**4. Keys and buttons while online**

| Action | Online call |
|---|---|
| E (lever) | `online_.interact();` instead of `game_.interact()` |
| Escape (pause / resume) | `online_.request(ControlKind::pause)` / `resume`; show the pause screen while `online_.paused()` (either player can pause both) |
| RETRY | `online_.request(ControlKind::restart)` |
| NEXT LEVEL | host: `advance_campaign()` as now; the joiner loads the same level by itself. Hide it on the joiner (`online_.role() == OnlinePlay::Role::joiner`) |
| Joiner on the won/lost screen | go back to play when `game_.state()` is `playing` again (the host retried or moved on) |

The host plays fire and the joiner plays water; on each machine both key sets steer that one character.

**Build the game** (downloads SDL3 the first time):

```
cmake -S apps/ui_prototype -B build-ui -DGROUP2_FETCH_SDL=ON
cmake --build build-ui --config Debug --parallel
ctest --test-dir build-ui -C Debug --output-on-failure
```

On macOS the existing `apps/ui_prototype/README.md` command with Homebrew SDL3 works too.

## Session API (low level)

```cpp
auto session = engine::net::make_enet_session();
session->host(7777);                      // Host Session button (0 = any free port; read it with port())
session->join("192.168.1.20", 7777);      // Join Session button with the typed address
// every frame: show session->state() and, when failed/disconnected, session->last_error()
```

| `state()` | Show the player |
|---|---|
| `hosting` | "Waiting for a player on port N" |
| `connecting` | "Connecting…" (gives up after 5 s) |
| `connected` | Both players present; enable Start Game |
| `failed` | `last_error()`: "Could not reach host", "Unknown host address: …", "Could not open port …" |
| `disconnected` | `last_error()`: "Connection lost" |

`host()`/`join()` return false and set `last_error()` when they fail at once. Call `close()` before hosting or
joining again.

## Drivers for other levels (Sydney, Blake)

`OnlinePlay` is built on two drivers that work with any level implementing `elemental_coop::NetLevel`
(tick, interact, restart, snapshot). `GameNetLevel` adapts `elemental_coop::Game` (used by `engine_demo` and the
tests); `DemoNetLevel` adapts the UI's `DemoLevel`.

```cpp
elemental_coop::GameNetLevel level(game);
elemental_coop::CoopHost host(level, *session);   // host: host.update(local_fire_input, dt) every frame
elemental_coop::CoopClient client(*session);      // joiner: client.update(local_water_input, lever_pressed)
```

Create exactly **one** driver per session; two drivers on one session steal each other's messages. The joiner's
input only ever moves water; jumps and lever presses travel as counters, so a lost packet never loses one.

## Try it

Build as in [SETUP](SETUP.md), then open two terminals.

| | Windows | macOS / Linux |
|---|---|---|
| Host | `.\build\Debug\engine_demo.exe --host 7777` | `./build/engine_demo --host 7777` |
| Join (same machine) | `.\build\Debug\engine_demo.exe --join 127.0.0.1 7777` | `./build/engine_demo --join 127.0.0.1 7777` |
| Join (other machine) | replace `127.0.0.1` with the host's local IP (`ipconfig` / `ipconfig getifaddr en0`) | |

Both print "Connected", then both characters' positions once per second. The host ends the session after 5 s
of play and the joiner reports "Connection lost". Close either window early to see the disconnect message.

**Firewall:** the first time, Windows asks whether `engine_demo.exe` (and `test_net_session.exe`) may use the
network. Allow **private networks**. macOS may ask the same.

**Demo network:** campus Wi-Fi often blocks traffic between devices. Use a phone hotspot or an ethernet cable
between the two laptops. Fallback: two windows on one laptop. Local co-op stays the playable product if
networking fails on the day.

## What is tested

`ctest --test-dir build -C Debug --output-on-failure -R "net_session|coop_net"` (about 3 s).

- Bytes and messages: every message type survives a round trip; wrong version, type, length, NaN or
  out-of-range values are rejected without throwing or changing the game.
- Real ENet on this machine: connect, messages both ways, a third player is not admitted, a second
  `host()`/`join()` does not break a live connection, closing is seen by the other side, a silent peer is
  reported lost after 2–3 s.
- On a simulated bad link (about 100 ms delay, 5% loss): every snapshot the joiner holds equals the host's state
  at that tick (NFR-18); a lost jump or lever press still happens once; pause, restart and disconnect work;
  reconnecting works.
- The playable game (`ctest --test-dir build-ui -C Debug -R ui_net`): with the real Level 1 the joiner pulls the
  lever and stands on the plate across the network and both machines see the gate open; on the bad link with
  Level 2's moving enemies, both machines show identical players, gates and enemies once paused; the joiner
  follows the host to the next level; a leaving joiner is reported as "Connection lost".

Measured by hand with the demo (Windows, two processes): a killed host or joiner is reported after
2.1–2.5 s (NFR-17 asks for 3 s). ENet's own timeout took 4.4–4.8 s, which is why the session uses a 2.5 s
silence watchdog instead. Joining an unused port reports "Could not reach host" after 5 s.

## Limits

- Exactly two players, LAN only: no internet play, NAT traversal, matchmaking or accounts.
- No client-side prediction: on a LAN the joiner's own input takes a few milliseconds to show.
- No interpolation: snapshots arrive every frame, which looked smooth in testing.
