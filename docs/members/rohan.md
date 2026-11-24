# Rohan — User Interaction & UI

Secondary responsibility: Artist.

## Implementation

InputBuffer supports a held movement axis and one-shot actions. Use a separate buffer per player. The [SDL app](../../apps/ui_prototype/README.md) plays seven gameplay and menu sounds. It loads WAV files through the shared file cache and respects the saved volume and effects settings.

- Public code: `engine/include/engine/interaction.hpp`
- Implementation: `engine/src/interaction.cpp`
- Tests: `tests/interaction_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R '^interaction$' --output-on-failure` (use double quotes in Windows shells).
