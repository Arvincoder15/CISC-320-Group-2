# Build setup

Use Git, CMake 3.20 or newer, and a C++20 toolchain. On Windows, Visual Studio 2022 with the Desktop development with C++ workload provides a suitable compiler. On macOS, install the Xcode Command Line Tools. On Linux, install GCC or Clang and the development packages needed to build SDL with your display and audio backends. The [rendering workflow](../.github/workflows/rendering.yml) lists the Linux packages used in CI.

## Graphical game

The [main README](../README.md#build-and-run) gives the standard commands. For an installed SDL 3.2 or newer, use `-DGROUP2_FETCH_SDL=OFF` and provide its prefix if CMake cannot find it. For example, with SDL3 installed through Homebrew:

```sh
cmake -S apps/ui_prototype -B build-ui -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON -DGROUP2_FETCH_SDL=OFF -DCMAKE_PREFIX_PATH="$(brew --prefix sdl3)"
cmake --build build-ui --config Debug --parallel
```

On Windows, a single-configuration generator such as Ninja places `ui_prototype.exe` directly in `build-ui`. For a single-configuration Release build, set `-DCMAKE_BUILD_TYPE=Release` at configure time. With Visual Studio, choose `--config Release` at build time and use the `Release` output directory.

## Headless engine and demos

From the repository root:

```sh
cmake -S . -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

This configuration builds the engine, the command-line demos, the relay, and the core tests. ENet is fetched at initial configuration. See [rendering](RENDERING.md) for SDL-specific targets.
