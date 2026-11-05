# Resource loading foundation — Arvin

## What now works

`FileResourceManager` loads binary file data from an existing asset root and reuses a cached, immutable `FileResource` for repeated requests. It composes the existing generic `ResourceCache`; it does not replace SDL texture/audio loaders or parse JSON/Tiled data.

```cpp
#include "engine/resource_files.hpp"

engine::FileResourceManager resources("assets");
auto config = resources.load("config/elemental_coop.example.json");
// config->bytes contains raw data, including any embedded zero bytes.
// A separate JSON adapter must parse and validate these bytes.
auto same = resources.load("config/./elemental_coop.example.json");
// same == config
resources.clear();
// config and same still own the data; it is released after the final handle.
```

The asset root is resolved at construction, so later working-directory changes do not change it. Applications should supply a root appropriate for their launch location; the example assumes the repository root is the working directory.

## Contract

- The root must be an existing accessible directory.
- Keys are nonempty relative filesystem paths. Absolute paths, embedded NUL characters and paths resolving outside the root are rejected.
- Canonical paths normalize `.` / `..` and existing symlinks. Aliases resolving to the same path share a cache entry. Hard links and differently cased names are not promised to deduplicate.
- Only regular files are accepted; an empty file is valid. Binary bytes are preserved.
- Path/read failures throw `ResourceError` with the relevant path and reason. Failed loads do not enter the cache and may be retried. Allocation failures retain their normal C++ exception types.
- Successful loads are snapshots: modifying a source file does not refresh cached bytes. `clear()` followed by `load()` reads again, while outstanding handles keep their old data alive.
- The manager and cache are single-threaded. Path checks assume trusted, stable project assets; they are not a security sandbox against concurrent filesystem changes.
- The caller decides whether to stop startup or use a fallback. The loader does not choose game-specific fallback behavior or silently log-and-ignore errors.

## Ownership and collaborators

The manager owns cached handles; consumers share immutable handles. Destroying an entity or the manager does not invalidate a handle held elsewhere. Clearing cached bytes is not a substitute for proper SDL teardown: decoded textures/audio need their own typed cache and matching deleters.

Coordinate with Aryaman for textures and renderer lifetime, Rohan for audio, and Gunveer for file/JSON utilities. Keep level interpretation with Gunveer and gameplay rules with Blake. No ECS interfaces were changed.

## Tests and next steps

The `resource_files` suite uses isolated temporary file fixtures: multi-chunk binary data, empty files, normalized paths, missing-file retry, invalid roots/keys, directory rejection, cache refresh and handles surviving clear/manager destruction. Symlink checks run where the OS permits their creation and print a skip otherwise. Temporary fixture directories are removed by RAII.

The infrastructure suite now also tests exception retry and a custom deleter invoked exactly once after final release. These tests model resource ownership without claiming actual SDL resource integration.

On November 5, 2026, all twelve CTest suites passed in both Debug and Release using CMake 4.4.4 and Apple Clang 15 / C++20. The infrastructure and game demos ran successfully in Debug. Windows/Linux and actual SDL loaders remain unverified. See [Arvin's implementation guide](ARVIN_INFRASTRUCTURE.md) for configuration and logging integration.

RAD mapping: FR-26 (foundation for assets), NFR-04 (cache/lifetime), NFR-05 (errors), NFR-12 (tests). Full FR-26 completion still requires real texture/audio adapters. Engine JSON configuration and severity-filtered logging are now implemented; game/level schemas and SDL adapters remain separate integration work.
