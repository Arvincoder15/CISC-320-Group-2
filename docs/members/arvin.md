# Arvin — Engine Infrastructure

Secondary responsibility: Software Developer/Architect.

## Implementation

Logger, typed ResourceCache and a FileResourceManager for cached binary files work without SDL. See [resource management](../RESOURCE_MANAGEMENT.md) for ownership, errors and tests. Validated engine JSON configuration and severity-filtered logging are implemented. The SDL game uses the file cache for font and audio assets. See the [infrastructure guide](../ARVIN_INFRASTRUCTURE.md).

- Public code: `engine/include/engine/infrastructure.hpp`
- File service: `engine/include/engine/resource_files.hpp` and `engine/src/resource_files.cpp`
- File-service tests: `tests/resource_files_test.cpp`
- Implementation: `engine/src/infrastructure.cpp`
- Tests: `tests/infrastructure_test.cpp`
- Build: follow [SETUP](../SETUP.md).
- Run the tests: `ctest --test-dir build -C Debug -R "^(infrastructure|resource_files|configuration)$" --output-on-failure`.
