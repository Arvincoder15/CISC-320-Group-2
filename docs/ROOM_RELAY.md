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

Use a Linux VM with systemd, a public IPv4 address, and inbound UDP support.
An Oracle Cloud Always Free-eligible VM is one option if your home region has
capacity. Select an eligible shape and keep the instance within the free limits.
Allow stateful IPv4 ingress for **UDP destination port 7777**, with any source
port, in the instance's network security group or subnet security list. For a
private team test, limit the source addresses to the players' public IPv4
addresses. The VM's own firewall also needs to permit this traffic.

On an Ubuntu VM, install the build tools and run the installer from the branch:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends git cmake build-essential
git clone --branch feature/arvin-room-code-relay https://github.com/Arvincoder15/CISC-320-Group-2.git
cd CISC-320-Group-2
bash deploy/install-room-relay.sh
```

The installer builds the headless relay, enables it at boot, and starts it under
a dedicated unprivileged systemd user. It stays up after SSH disconnects. Check
it with `systemctl status elemental-coop-relay.service` or view logs with
`sudo journalctl -u elemental-coop-relay.service -n 50`. Pull and rerun the
installer to update the relay between matches. Restarting it ends active rooms.

Both players launch the same game commit with
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
