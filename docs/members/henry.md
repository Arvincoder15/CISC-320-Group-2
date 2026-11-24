# Henry — Networking

Secondary responsibility: Repository Design/Maintenance.

## Implementation

Two-player networked play over ENet: connection and disconnect detection, the co-op protocol, and host/client
sync drivers, with a two-process demo. Usage and test coverage: [NETWORKING.md](../NETWORKING.md).

- Engine API: `engine/include/engine/networking.hpp` (`Session`, byte reader/writer, in-memory test pair)
- Implementation: `engine/src/networking.cpp`, `engine/src/enet_session.cpp`
- Game protocol and drivers: `games/elemental_coop/include/elemental_coop/net.hpp`, `games/elemental_coop/src/net.cpp`
- Playable game: `apps/ui_prototype/net_play.hpp` (`OnlinePlay`), the lobby and game loop in
  `apps/ui_prototype/main.cpp`, `DemoLevel::apply_network_state()` in `apps/ui_prototype/demo_level.hpp`
- Demo: `engine_demo --host <port>` / `engine_demo --join <address> <port>` (`apps/headless_demo.cpp`)
- Tests: `tests/net_session_test.cpp`, `tests/coop_net_test.cpp`, `apps/ui_prototype/net_test.cpp`
  (old queue: `tests/networking_test.cpp`)
- Run them: `ctest --test-dir build -C Debug -R "net_session|coop_net|networking" --output-on-failure` and,
  for the game, `ctest --test-dir build-ui -C Debug -R "ui_net|ui_game_smoke" --output-on-failure`
