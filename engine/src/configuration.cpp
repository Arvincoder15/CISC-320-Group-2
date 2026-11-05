#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <initializer_list>
#include <string>

namespace engine {
namespace {
using Json = nlohmann::json;
void require(bool valid, const std::string& message) {
    if (!valid) throw ConfigurationError(message);
}
const Json& section(const Json& document, const char* key) {
    static const Json empty = Json::object();
    if (!document.contains(key)) return empty;
    require(document.at(key).is_object(), std::string(key) + " must be an object");
    return document.at(key);
}
void dimension(const Json& object, const char* key, int& output) {
    if (!object.contains(key)) return;
    const auto& value = object.at(key);
    require(value.is_number_integer(), std::string("window.") + key + " must be an integer");
    const auto number = value.get<double>();
    require(number >= 1 && number <= 16384, std::string("window.") + key + " must be in [1, 16384]");
    output = value.get<int>();
}
void boolean(const Json& object, const char* key, bool& output, const char* prefix) {
    if (!object.contains(key)) return;
    require(object.at(key).is_boolean(), std::string(prefix) + "." + key + " must be a boolean");
    output = object.at(key).get<bool>();
}
void known_fields(const Json& object, std::initializer_list<std::string_view> allowed, const char* prefix) {
    for (const auto& [key, value] : object.items()) {
        (void)value;
        bool known = false;
        for (const auto field : allowed) if (key == field) known = true;
        require(known, std::string(prefix) + "." + key + " is not a supported setting");
    }
}
}
EngineSettings parse_engine_settings(std::string_view text, std::string_view source) {
    try {
        const auto document = Json::parse(text.begin(), text.end());
        require(document.is_object(), "root must be an object");
        require(document.contains("schema_version") && document.at("schema_version").is_number_integer()
                && document.at("schema_version") == 1, "schema_version must be integer 1");
        EngineSettings result;
        const auto& window = section(document, "window");
        const auto& audio = section(document, "audio");
        known_fields(window, {"width", "height", "fullscreen"}, "window");
        known_fields(audio, {"master_volume", "sound_effects"}, "audio");
        dimension(window, "width", result.window.width);
        dimension(window, "height", result.window.height);
        boolean(window, "fullscreen", result.window.fullscreen, "window");
        boolean(audio, "sound_effects", result.audio.sound_effects, "audio");
        if (audio.contains("master_volume")) {
            const auto& value = audio.at("master_volume");
            require(value.is_number(), "audio.master_volume must be a number");
            const auto volume = value.get<double>();
            require(std::isfinite(volume) && volume >= 0 && volume <= 1, "audio.master_volume must be in [0, 1]");
            result.audio.master_volume = static_cast<float>(volume);
        }
        return result;
    } catch (const Json::exception& error) {
        throw ConfigurationError("Configuration '" + std::string(source) + "': invalid JSON: " + error.what());
    } catch (const ConfigurationError& error) {
        throw ConfigurationError("Configuration '" + std::string(source) + "': " + error.what());
    }
}
std::string serialize_engine_settings(const EngineSettings& settings) {
    require(settings.window.width >= 1 && settings.window.width <= 16384,
            "window.width must be in [1, 16384]");
    require(settings.window.height >= 1 && settings.window.height <= 16384,
            "window.height must be in [1, 16384]");
    require(std::isfinite(settings.audio.master_volume)
            && settings.audio.master_volume >= 0 && settings.audio.master_volume <= 1,
            "audio.master_volume must be finite and in [0, 1]");
    const Json document = {
        {"schema_version", 1},
        {"window", {{"width", settings.window.width}, {"height", settings.window.height},
                    {"fullscreen", settings.window.fullscreen}}},
        {"audio", {{"master_volume", settings.audio.master_volume},
                   {"sound_effects", settings.audio.sound_effects}}}
    };
    return document.dump(2) + '\n';
}
EngineSettings load_engine_settings(const std::filesystem::path& root, const std::filesystem::path& key) {
    FileResourceManager files(root); // Fresh read; no stale configuration cache on reload.
    const auto file = files.load(key);
    const std::string text(file->bytes.begin(), file->bytes.end());
    return parse_engine_settings(text, file->source.generic_string());
}
}
