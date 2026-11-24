# Sydney — Engine Architecture / ECS

Secondary responsibility: Repository Design/Maintenance.

## Implementation

`World` manages entity lifetimes and typed component storage with stable entity IDs. The SDL campaign uses it for player transforms, physics bodies, colliders, and contact state.

- Public code: `engine/include/engine/architecture.hpp`
- Implementation: `engine/src/architecture.cpp`
- Tests: `tests/architecture_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R '^architecture$' --output-on-failure` (use double quotes in Windows shells).
