# Elemental Co-op

Elemental Co-op is a two-player puzzle platformer built with the reusable C++20 engine developed by CISC 320 Group 2. Play as Fire and Water, dodge elemental hazards and acid slimes, and work together to open gates and reach the matching exits across three levels.

## Build and run

Install Git, CMake 3.20 or newer, and a C++20 compiler. Run these commands from the repository root. The first configuration downloads SDL3 and ENet.

```sh
cmake -S apps/ui_prototype -B build-ui -DGROUP2_FETCH_SDL=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-ui --config Debug --parallel
```

- **macOS / Linux** — `./build-ui/ui_prototype`
- **Windows with Visual Studio** — `.\build-ui\Debug\ui_prototype.exe`

Keep the generated `assets` and `rendering-assets` directories beside the executable. See [build setup](docs/SETUP.md) for an installed SDL3 build and the headless engine targets.

## Play

Select **Local Play** for two players on one keyboard.

| Action | Key |
| --- | --- |
| Move and jump as Fire | A / D and W |
| Move and jump as Water | Left / Right and Up |
| Use a nearby lever | E |
| Pause or resume | Esc |
| Toggle background music | M or the music icon |

For two computers on the same LAN, open **Multiplayer** on both. One player selects **Host Session** and controls Fire. The other enters the host's LAN IP and selects **Join Session** to control Water. The host selects **Start Game** once both players are connected. With two windows on one computer, use `127.0.0.1`.

For players on different networks, use **Create Room** and **Join Room** with both builds configured for the same reachable UDP relay. See [room setup and deployment](docs/ROOM_RELAY.md).

## Test

```sh
ctest --test-dir build-ui -C Debug --output-on-failure
```

See [testing](docs/TESTING.md) for the automated coverage and manual checks.

## Project guide

- [Architecture](docs/ARCHITECTURE.md) and [game rules](docs/GAME_DESIGN.md)
- [SDL application, level data, and developer tools](apps/ui_prototype/README.md)
- [Team responsibilities and subsystem notes](docs/TEAM.md)
