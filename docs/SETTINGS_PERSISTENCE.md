# Settings persistence — step 1

Branch: `feature/arvin-settings-persistence`, based on the merged infrastructure work.

The first step is serialization: converting the settings in C++ memory into JSON text.

```cpp
engine::EngineSettings settings;
settings.window.width = 1280;
settings.audio.master_volume = 0.5F;
const std::string json = engine::serialize_engine_settings(settings);
// json is text in memory. No file has been written yet.
```

The serializer writes schema version 1 and every supported window/audio field. It rejects invalid dimensions and invalid/non-finite volume values, using the same accepted ranges as loading. A round-trip test serializes settings, parses that text again, and checks every field. This checks that saving the representation will not lose settings.

The result is a standalone engine settings document. It does not contain an existing file's `game` section or other data; do not use it to overwrite a shared configuration file yet.

Next small steps:

1. Choose a writable user-settings location with Rohan; packaged assets may be read-only.
2. Add a file-saving API that preserves unrelated fields when updating shared JSON.
3. Write to a temporary file and safely replace the destination; test that failures preserve the previous file.
4. Connect the settings screen's Apply action and load persisted settings on startup.

No disk-writing API, automatic save or settings-screen integration is implemented in step 1.
