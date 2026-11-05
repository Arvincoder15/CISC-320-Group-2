# Settings persistence — step 2

Branch: `feature/arvin-settings-persistence`, based on the merged infrastructure work.

Serialization converts settings into JSON text. File saving now validates that data,
then writes it to an application-selected location. The application must choose a
writable user-settings folder and create that folder before saving; packaged assets
may be read-only.

```cpp
engine::EngineSettings settings;
settings.window.width = 1280;
settings.audio.master_volume = 0.5F;

// user_settings_directory is an existing writable folder supplied by the app.
const auto path = user_settings_directory / "settings.json";
try {
    engine::save_engine_settings(path, settings);
    const auto restored = engine::load_engine_settings(user_settings_directory, "settings.json");
} catch (const engine::ConfigurationError& error) {
    // Report invalid settings or a save failure to the user through logging/UI.
} catch (const engine::ResourceError& error) {
    // Report a file-read failure (include engine/resource_files.hpp for this type).
}
```

## Save behavior

- New files contain schema version 1 and all supported window/audio settings.
- Existing files must be valid schema-v1 configurations. Saving replaces their
  window/audio sections and preserves other top-level values, including `game`.
  Whitespace and key ordering may change; this is not a text-preserving editor.
- Invalid settings, corrupt existing files, unsupported schemas, symlink
  destinations and directory destinations are rejected with `ConfigurationError`.
  Errors include the requested destination path.
- A uniquely created sibling temporary directory holds the complete new file.
  The stream is closed and checked before filesystem rename replaces the old file.
  The old file is never deleted first. If replacement is refused, saving reports an
  error. The temporary directory is cleaned up on normal return and exceptions.

The API assumes one writer and a trusted, stable user-settings directory. It does
not coordinate concurrent processes or guarantee durability after power loss.
Abrupt process termination can leave a temporary directory. Filesystem replacement
behavior has been exercised on macOS; Windows/Linux verification remains pending.

`serialize_engine_settings` still produces standalone JSON text only. Use the save
API when updating a shared document so unrelated sections are retained.

## Verification

All 12 Debug CTest suites pass on macOS. Configuration tests cover new-file and
replacement save/load round trips, all five settings fields, unrelated sections,
invalid settings leaving original bytes untouched, corrupt/unsupported documents,
missing parent directories, rejected directory/symlink destinations, and temporary
file cleanup. Disk-full and interrupted-write failures are not fault-injected.

## Next small steps

1. Agree on the writable user-settings location with Rohan.
2. Load that file on startup, using defaults on first launch and reporting errors.
3. Connect the settings screen's Apply action to saving and live window/audio changes.

No startup integration or settings-screen integration is implemented yet.
