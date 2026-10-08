# Cooperative starter validation — October 8, 2026

Executed locally on macOS arm64 with Apple Clang 15.0.0 and CMake 4.4.4:

- Debug configure/build: passed with the configured warning flags.
- Debug CTest: 10/10 suites passed.
- Release configure/build: passed with the configured warning flags.
- Release CTest: 10/10 suites passed.
- Headless demo: completed 120 ticks; two players, two rendered commands, two cached resources, cooperative exit reached.
- Updated gameplay checks: independent motion, grounded-only jump and landing, all six player/hazard combinations, shared failure, repeated restart, same-step exit completion, quit and invalid input.
- Local Markdown links and example JSON syntax: passed.

CMake was downloaded into `/private/tmp/group2-cmake-tools` for validation because it was not on this machine's PATH. This is temporary tooling, not a project dependency or a permanent CMake installation. The executable used was `/private/tmp/group2-cmake-tools/cmake/data/bin/cmake`; CTest is beside it. For future development install CMake normally as described in SETUP.md, or use those absolute paths while they exist.

Windows/Linux builds and the GitHub Actions workflow have not been executed in this session. SDL, JSON, ImGui, Tiled and real networking are not integrated and were not tested. These results cover the starter code only.
