#pragma once
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace engine {
struct WindowSettings {
    int width{960};
    int height{540};
    bool fullscreen{false};
};
struct AudioSettings {
    float master_volume{0.8F};
    bool sound_effects{true};
};
struct EngineSettings {
    WindowSettings window{};
    AudioSettings audio{};
};
class ConfigurationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
// Parses engine settings only. Other top-level sections (e.g. game) are left to
// their owners. Requires schema_version=1; omitted window/audio fields default.
EngineSettings parse_engine_settings(std::string_view json, std::string_view source = "<memory>");
// Validates and converts engine settings to a standalone schema-v1 JSON document.
// Does not write a file or preserve other sections from an existing document.
std::string serialize_engine_settings(const EngineSettings& settings);
// Saves to an application-selected writable path whose parent already exists.
// Preserves unrelated top-level sections in an existing valid schema-v1 file.
// Rejects symlinks and invalid existing documents. Errors are ConfigurationError.
// Single writer only; no guarantee of durability across power loss.
void save_engine_settings(const std::filesystem::path& path, const EngineSettings& settings);
enum class SettingsLoadStatus { loaded, missing, failed };
struct SettingsLoadResult {
    EngineSettings settings{};
    SettingsLoadStatus status{SettingsLoadStatus::missing};
    std::string diagnostic; // Empty for loaded/missing; report failures through logging/UI.
};
// Startup policy: use validated defaults when absent or unreadable/invalid.
// Does not create folders/files or overwrite a rejected file. Invalid defaults throw.
SettingsLoadResult load_user_settings(const std::filesystem::path& path,
                                     const EngineSettings& defaults = {});
// Reads fresh on every call. Returns a complete value; never mutates live settings.
// File/path errors are ResourceError; schema/JSON errors are ConfigurationError.
EngineSettings load_engine_settings(const std::filesystem::path& asset_root,
                                    const std::filesystem::path& relative_path);
}
