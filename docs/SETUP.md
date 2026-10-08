# Local setup

## Prerequisites

- Git and CMake 3.20 or newer.
- A C++20-capable compiler and its platform SDK/build tools. Exact minimum compiler versions are pending team verification.
- macOS: Apple Command Line Tools (Apple Clang) and CMake.
- Windows: Visual Studio C++ desktop build tools, a Windows SDK and CMake; use a developer terminal.
- Linux: GCC or Clang, development headers, Make (or Ninja) and CMake.

Check `cmake --version` and your compiler (`clang++ --version`, `g++ --version`, or `cl`). Install missing tools using your platform's normal tooling. Clone the shared repository and run the following from its root.

## Configure, build, test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

On macOS/Linux with a single-configuration generator:

```sh
./build/engine_demo
```

In Windows PowerShell with the Visual Studio generator:

```powershell
.\build\Debug\engine_demo.exe
```

Success means ten passing CTest suites and a demo summary with 120 ticks, two players, two rendered commands and two cached resources. There is no window yet. Each owner's handoff shows how to run only their suite.

For a separate optimized build:

```sh
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release --parallel
ctest --test-dir build-release -C Release --output-on-failure
```

CMake ignores `CMAKE_BUILD_TYPE` for multi-configuration generators such as Visual Studio; `--config` and `-C` choose the configuration there. Use a new build directory when changing compilers or generators.

## Adding a source or test

Add new `.cpp` files to the owning target in `CMakeLists.txt` with `target_sources`. Public headers belong under the owning include directory. Keep engine targets independent of the cooperative game. Register additional test executables with `add_test`; extend the corresponding owner suite for related small cases. Do not use C `assert()` as the test runner because Release builds can disable it.

`BUILD_TESTING=OFF` builds the demo without tests. The small runner in `tests/test_support.hpp` is temporary. Once the team chooses GoogleTest or Catch2, Arvin can migrate it while retaining the behavioral cases.

## Introducing the intended stack

Dependency versions and acquisition strategy are deliberately unresolved. No automatic fetch occurs during configuration. For each dependency: record a reviewed version/license, choose installed packages or a pinned fetch method, add its target to the owning module, then verify Windows/macOS/Linux CI.

- **SDL3:** Aryaman starts the renderer/window backend with Rohan agreeing who polls events. Arvin supplies resource loader adapters and shutdown tests.
- **JSON:** Arvin proposes the shared parser/file service; Gunveer owns the level schema/importer; Henry owns protocol serialization.
- **Dear ImGui:** Gunveer integrates tools using Aryaman's renderer and Rohan's event dispatch.
- **Tiled:** Gunveer documents the supported exported format/subset; unsupported maps fail clearly.
- **GoogleTest/Catch2:** team choice, implemented by Arvin. CTest remains the top-level command.

Official build references: [CMake compile features](https://cmake.org/cmake/help/latest/command/target_compile_features.html) and [CTest registration](https://cmake.org/cmake/help/v3.20/command/add_test.html).
