# Settings persistence — step 3

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

## Startup loading

`load_user_settings(path, defaults)` returns a `SettingsLoadResult` containing a
complete settings value, a status (`loaded`, `missing`, or `failed`) and a diagnostic.
Missing files, including a missing first-launch folder, use the supplied defaults
without writing anything. Invalid JSON, unsupported schemas, invalid values and
file-access failures also use defaults, with a diagnostic the caller should report.
The rejected file is preserved. Invalid caller-provided defaults throw
`ConfigurationError` because they indicate a programming/configuration mistake.
Symlink and directory destinations are rejected consistently with saving.

```cpp
const auto startup = engine::load_user_settings(user_settings_directory / "settings.json");
const auto settings = startup.settings;
if (startup.status == engine::SettingsLoadStatus::failed) {
    logger.write(engine::LogLevel::warning, "configuration", startup.diagnostic);
}
```

The infrastructure demo accepts an optional path to exercise startup behavior:

```sh
./build/infrastructure_demo assets /path/to/user-settings/settings.json
```

It uses the packaged example configuration as defaults, then loads the optional
user file and reports the resulting settings. The demo does not automatically save.
Tests exercise missing folders, successful loading, malformed/schema/value errors,
directory/symlink rejection, invalid defaults, and preservation of rejected files.

## Integration handoff

1. Agree on the writable user-settings location with Rohan. The engine accepts an
   application-supplied path so it does not hardcode an operating-system folder.
2. Call the startup helper from the graphical application's initialization and
   apply the returned values to its actual window and audio device.
3. Connect Apply to saving and live window/audio changes. Create the writable
   parent folder before the first save, report save failures, and allow the user
   to repair/reset corrupt saved settings explicitly.

The startup policy is implemented and demonstrated; graphical application and
settings-screen integration remain pending. Main's latest ECS, AI and physics
changes have been merged into this branch without conflicts.
