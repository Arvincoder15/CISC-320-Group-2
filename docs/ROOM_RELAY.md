# Elemental Co-op remote multiplayer

Both games connect outward to a relay. Fire creates the room and runs the
match. Water joins by code and receives the game state. The relay forwards ENet
messages between the pair. Direct-IP play remains available by default.

## Test from this branch

Build the game and relay together:

```sh
cmake -S apps/ui_prototype -B build-ui -DGROUP2_FETCH_SDL=ON
cmake --build build-ui --config Debug --parallel
ctest --test-dir build-ui -C Debug --output-on-failure
```

With Homebrew SDL3, you can configure with `-DGROUP2_FETCH_SDL=OFF` and
`-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/sdl3` on an Apple Silicon Mac.

1. Run `./build-ui/engine-build/room_relay` in one terminal.
2. Run `ELEMENTAL_COOP_RELAY=127.0.0.1 ./build-ui/ui_prototype` in each of two other terminals.
3. In Multiplayer, select **Create Room**, then copy the displayed code.
4. In the other window, enter or paste the code and select **Join Room**.
5. Select **Start Game** in the host window. Verify Fire and Water move independently.

With Visual Studio use `build-ui/engine-build/Debug/room_relay.exe` and
`build-ui/Debug/ui_prototype.exe`. Set the endpoint with
`$env:ELEMENTAL_COOP_RELAY = "127.0.0.1"` in PowerShell before opening the game.

The `ui_room_relay` check uses real ENet sockets on a local ephemeral port.
It covers room creation, joining, both players' movement, a third player
being rejected, disconnect, and retry.

## Deploy for two different networks

On a Linux machine with a public IPv4 address and inbound UDP support, build
and run the headless relay as an unprivileged user:

```sh
cmake -S . -B build-relay -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-relay --target room_relay --parallel
./build-relay/room_relay 7777
```

Allow inbound UDP 7777 through the machine and provider firewalls. Keep the
relay running while playing. Both players launch the same game commit with
`ELEMENTAL_COOP_RELAY` set to the relay hostname or IPv4 address. A packaged
build can set `-DGROUP2_RELAY_ADDRESS=relay.example.com` when configuring the UI,
so players can open the game normally. An empty `ELEMENTAL_COOP_RELAY`
environment override selects direct-IP mode. GitHub Pages can link to the game
build but cannot run this relay.

Test with one computer on Wi-Fi and one on a phone hotspot, with any VPN turned
off. Verify both characters, a shared puzzle action, and leaving the match. The
local relay test cannot verify reachability or latency on separate networks.

## Scope

Room codes are single-use invitations and expire after ten minutes before a
join. The match ends if either player leaves. ENet traffic is unencrypted and
the relay has no player accounts or operator controls. Use it for the intended
test group. The host's game stays open to run the match.
