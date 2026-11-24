# Networking design records

Henry drafted these ADRs on November 4, 2026. They record the design proposals made at the time. See [networking](NETWORKING.md) for the shipped behavior and [room relay](ROOM_RELAY.md) for deployment.

### ADR NET-1: Topology and authority

Status: Proposed
Date: 2026-11-04
Owner / reviewers: Henry / Blake, Sydney
Context and problem: The RAD requires networked two-player play between separate computers (FR-NET-01..05), with the server's state winning disagreements. It does not say whether the server is a third program or part of a player's game.
Options considered: (a) dedicated server process plus two clients; (b) listen server, where the host's game also runs the server; (c) peer-to-peer lockstep.
Decision and rationale: (b) listen server. The host's game runs the only simulation and plays slot 0 (fire). The joining player is slot 1 (water). Clients send input only and display the snapshots they receive, so they never run gameplay rules and cannot overwrite authoritative state. Two programs instead of three matches the lobby's Host/Join design, and lockstep would require bit-identical simulation on every platform.
Scope limits: exactly two players; LAN only (no NAT traversal, matchmaking, accounts or internet play); no client-side prediction. On a LAN the joiner's round-trip delay is a few milliseconds.
API, ownership and dependency consequences: The host owns `Game`. The client needs no `Game` until Blake provides `Game::apply_snapshot()`; until then it renders from the latest snapshot.
Tests / migration required: two-process demo on localhost and on two LAN machines.
Jira and Confluence links: NET-01 (key pending)

### ADR NET-2: Transport library

Status: Proposed
Date: 2026-11-04
Owner / reviewers: Henry / Arvin (dependencies), Sydney (repository)
Context and problem: We need connections, disconnect detection, reliable delivery for control messages and unreliable delivery for per-tick state on Windows, macOS and Linux.
Options considered: raw BSD/Winsock sockets (we would have to write reliability, timeouts and two OS code paths); Asio (large, still needs our own reliability layer); Valve GameNetworkingSockets (needs protobuf and OpenSSL); SDL3_net (thin socket wrapper, no reliability layer); ENet.
Decision and rationale: ENet v1.3.18 (MIT licence, plain C, about ten source files), fetched with CMake `FetchContent` at a pinned tag. It provides connect/disconnect events with configurable timeouts (needed for NFR-17), reliable and unreliable channels, and peer limits, and it builds on all three CI platforms.
API, ownership and dependency consequences: `engine_networking` links ENet privately; no other target or header sees ENet. The first configure needs internet access to fetch ENet.
Tests / migration required: a test that creates an ENet host; CI on all three platforms.
Jira and Confluence links: NET-02, NET-04 (keys pending)

### ADR NET-3: Wire protocol

Status: Proposed
Date: 2026-11-04
Owner / reviewers: Henry / Blake
Context and problem: Messages must be explicit bytes, never raw struct memory, and invalid input from the network must never crash the game or change its state.
Decision and rationale:
- Every message starts with a protocol version byte (`2`; version 1 lacked gates, enemies and lever input) and a type byte. Integers and floats are little-endian; floats are IEEE-754 32-bit. Fixed messages have one exact size; snapshot lists carry a count byte with an upper limit. Any other size is rejected.
- Channel 0 is reliable and ordered (control). Channel 1 is unreliable and sequenced (input and snapshots), so a late packet is dropped rather than delaying newer ones.

| Type | Direction / channel | Fields |
|---|---|---|
| Hello | client → host, reliable | none |
| Welcome | host → client, reliable | slot u8 (always 1) |
| Reject | host → client, reliable | reason u8 (version mismatch, session full) |
| Input | client → host, unreliable | tick u32, move_x f32 in [-1, 1], jump_count u8, interact_count u8 |
| Snapshot | host → client, unreliable | tick u32, round_state u8, level u8; for each player: x, y, vx, vy f32 and flags u8 (grounded, alive, at_exit, facing_left); gate count u8 (≤ 8) and per gate flags u8 (lever_on, plate_occupied, open); enemy count u8 (≤ 16) and per enemy x, y, animation_time f32 and flags u8 (facing_left) |
| Control | either way, reliable | kind u8: pause, resume, restart, quit |

- Jumps travel as a counter (`jump_count`) that increases on each press. A lost Input packet loses no jump, because the next packet carries the new count. The host fires one jump each time the count changes.
- Validation at the host: the sender's slot comes from the connection, never from the packet; input with a tick at or below the last accepted tick is dropped; non-finite or out-of-range values are rejected. The client drops snapshots older than the newest one it has.
- Lever presses travel as a second counter (`interact_count`) with the same rule as jumps.
- The host sends one snapshot per frame: 44 bytes with no gates or enemies, at most 260 bytes at the limits (the hardest campaign level, 2 gates and 5 acid enemies, is 111 bytes).
- Gates and enemies are identified by their order in the level file, and the level by its campaign index, so both machines agree without sending `EntityId` values.

API, ownership and dependency consequences: Message encoding lives in the game layer (`games/elemental_coop`); byte reading and writing helpers live in `engine_networking`. Arvin's planned reusable serializer (FR-INF-08) may later absorb those helpers.
Tests / migration required: round trip of every message type; rejection of wrong version, type, length, NaN and out-of-range axis; stale and wrong-slot input ignored; lost-jump test.
Jira and Confluence links: NET-03, NET-06 (keys pending)

### ADR NET-4: Engine/game boundary and the Session API

Status: Proposed
Date: 2026-11-04
Owner / reviewers: Henry / Sydney, Blake, Rohan
Context and problem: Engine code must not depend on game code, and teammates need a small API that hides ENet.
Decision and rationale:
- `engine::net::Session` (engine) moves bytes for exactly one peer: `host(port)`, `join(address, port)`, `send(channel, bytes)`, `poll()` returning connected/disconnected/message events, `state()`, `last_error()`, `port()`, `close()`. It knows nothing about fire, water, doors or rounds.
- Two implementations: ENet for real play, and an in-memory pair with optional delay and packet loss so tests run without sockets.
- The game layer (`elemental_coop` networking files, owned by Henry, reviewed by Blake) defines the messages and two drivers: `CoopHost` (owns the tick, reads `Game` through its existing public accessors, applies remote input, sends snapshots) and `CoopClient` (sends input, keeps the newest snapshot).
- Rohan's lobby only calls `host`/`join` and reads `state()` and `last_error()`. Blake's only future task is `Game::apply_snapshot()`.

API, ownership and dependency consequences: The existing `StateSnapshot`/`LoopbackTransport` stay for the current demo and integration test until they are replaced.
Tests / migration required: session tests on both implementations; drivers tested over the in-memory pair.
Jira and Confluence links: NET-03, NET-06 (keys pending)

### ADR NET-5: Test conditions, failure behaviour and demo setup

Status: Proposed
Date: 2026-11-04
Owner / reviewers: Henry / Rocco (schedule), Blake
Context and problem: NFR-18 says "under agreed test conditions" without defining them, NFR-17 asks to notify both players even when one side's game has died, and the RAD leaves online pause undefined.
Decision and rationale:
- Test conditions for NFR-18: localhost, a LAN, and a simulated link with about 100 ms delay and 5% loss on the unreliable channel. Pass: after one second without input, the client's latest snapshot equals the host's state for the same tick.
- NFR-17 is reworded: the remaining player is notified within 3 seconds of losing the connection. ENet's built-in timeout was measured at 4.4–4.8 s (it only fires at exponential retry points), so the session uses its own watchdog: ENet pings every 0.5 s, and 2.5 s without any traffic counts as lost (measured 2.1–2.5 s). A third player trying to join is refused.
- Online pause and restart: either player can pause, which pauses both; only the host's game restarts, and the joiner's Restart sends a request.
- Demo network: a phone hotspot or ethernet link between two laptops, because campus Wi-Fi often blocks device-to-device traffic. Fallback: two game windows on one laptop. Local co-op remains the playable product if networking fails, as the RAD states.

Tests / migration required: disconnect, restart and pause tests over the in-memory pair; recorded two-machine run.
Jira and Confluence links: NET-07, NET-08 (keys pending)

### Requirement clarifications proposed for the RAD/SDD

- FR-NET-06 (server validates changes) becomes Must: with input-only clients it costs a few range and order checks.
- NFR-17 wording as in NET-5.
- Brainstorm items that do not apply to a two-player puzzle game are dropped from networking scope: synchronizing attacks, projectiles and health; more than two clients; client prediction and reconciliation.
- FR-NET-07 (interpolation) stays Could: snapshots every tick on a LAN are expected to look smooth without it.
