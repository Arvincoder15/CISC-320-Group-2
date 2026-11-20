# Networking (Henry)

Two players on two computers on the same local network play one co-op level. The player who hosts runs the
game and controls **fire** (slot 0); the player who joins controls **water** (slot 1). Design decisions and the
reasons behind them are in [DECISIONS.md](DECISIONS.md), ADRs NET-1 to NET-5.

**Status:** connection, protocol, host/client sync, disconnect detection and a two-process demo work and are
tested. Not yet done: plates/doors in the snapshot (waiting for them in `Game`), `Game::apply_snapshot` (Blake),
and hooking the drivers into the SDL game loop and lobby.

## The pieces

| What | Where | Who uses it |
|---|---|---|
| `engine::net::Session`: connect and move bytes for one other player | `engine/include/engine/networking.hpp` | Lobby (Rohan), game loop |
| ENet implementation, `make_enet_session()` | `engine/src/enet_session.cpp` | Real play |
| In-memory pair with delay/loss, `make_loopback_pair()` | `engine/src/networking.cpp` | Tests only |
| Co-op messages, `CoopHost`, `CoopClient`, `snapshot_of()` | `games/elemental_coop/include/elemental_coop/net.hpp` | Game loop (Blake, Sydney) |

ENet v1.3.18 is downloaded by CMake on the first configure, so that first configure needs internet. Nothing
outside `engine/src/enet_session.cpp` includes ENet.

## Lobby (Rohan)

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

## Game loop (Sydney, Blake)

Create exactly **one** driver per session; two drivers on one session steal each other's messages.

```cpp
// Host: owns the Game and runs it.
elemental_coop::CoopHost host(game, *session);
host.update(local_fire_input, dt);          // once per frame; ticks Game, sends a snapshot
if (host.client_ready()) { /* draw from game as in local play */ }

// Joiner: no Game needed yet.
elemental_coop::CoopClient client(*session);
client.update(local_water_input);           // once per frame; sends input, receives snapshots
if (const auto& s = client.latest()) { /* draw s->players[0] (fire) and s->players[1] (water) */ }
if (client.rejected()) { /* "Host runs a different version" */ }
```

- Pause, resume, restart, quit: `host.request(...)` or `client.request(...)` with `elemental_coop::ControlKind`.
  Either player can pause, and that pauses both; only the host's game restarts.
- The joiner's input only ever moves water. Input arrives as an axis plus a jump counter, so a lost packet never
  loses a jump.

**Blake:** when `Game` gets plates/doors, add them to `NetSnapshot` (see the `shortcut:` comment in `net.hpp`),
identified by their index in the level file. When `Game::apply_snapshot()` exists, the joiner can keep a `Game`
and apply `client.latest()` to it instead of drawing the snapshot directly.

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
  at that tick (NFR-18); a lost jump still jumps once; pause, restart and disconnect work; reconnecting works.

Measured by hand with the demo (Windows, two processes): a killed host or joiner is reported after
2.1–2.5 s (NFR-17 asks for 3 s). ENet's own timeout took 4.4–4.8 s, which is why the session uses a 2.5 s
silence watchdog instead. Joining an unused port reports "Could not reach host" after 5 s.

## Limits

- Exactly two players, LAN only: no internet play, NAT traversal, matchmaking or accounts.
- No client-side prediction: on a LAN the joiner's own input takes a few milliseconds to show.
- No interpolation: snapshots arrive every frame, which looked smooth in testing.
