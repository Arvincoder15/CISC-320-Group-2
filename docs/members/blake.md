# Blake — Gameplay Systems

Secondary responsibility: Software Developer/Architect.

## Implementation

The [shared game module](../../games/elemental_coop/README.md) supplies two independent player slots, elemental hazard rules, a shared restart, and the joint exit condition for the headless demo. The SDL campaign applies the cooperative rules across its playable levels.

- Public code: `games/elemental_coop`
- Implementation: `games/elemental_coop/src/game.cpp`
- Tests: `tests/gameplay_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R '^gameplay$' --output-on-failure` (use double quotes in Windows shells).
