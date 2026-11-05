#pragma once
#include <filesystem>
#include <stdexcept>
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
// Reads fresh on every call. Returns a complete value; never mutates live settings.
// File/path errors are ResourceError; schema/JSON errors are ConfigurationError.
EngineSettings load_engine_settings(const std::filesystem::path& asset_root,
                                    const std::filesystem::path& relative_path);
}
