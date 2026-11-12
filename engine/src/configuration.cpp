#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <chrono>
#include <fstream>
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
// An exclusively created sibling directory owns the temporary file. Keeping it
// beside the destination allows rename without crossing filesystem boundaries.
class PendingSettings {
public:
    explicit PendingSettings(const std::filesystem::path& parent) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = parent / (".engine-settings-" + std::to_string(stamp)
                                             + "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) {
                directory = candidate;
                return;
            }
        }
        throw ConfigurationError("Unable to create temporary settings directory");
    }
    ~PendingSettings() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
    PendingSettings(const PendingSettings&) = delete;
    PendingSettings& operator=(const PendingSettings&) = delete;
    std::filesystem::path file() const { return directory / "settings.json"; }
private:
    std::filesystem::path directory;
};
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
void save_engine_settings(const std::filesystem::path& path, const EngineSettings& settings) {
    try {
        // Validate before touching the filesystem.
        const auto updated = Json::parse(serialize_engine_settings(settings));
        require(!path.empty() && path.has_filename(), "a settings filename is required");
        const auto destination = std::filesystem::absolute(path);
        const auto parent = destination.parent_path();
        std::error_code error;
        const auto status = std::filesystem::symlink_status(destination, error);
        if (error && error != std::errc::no_such_file_or_directory)
            throw std::filesystem::filesystem_error("Cannot inspect destination", destination, error);
        require(status.type() == std::filesystem::file_type::not_found
                || std::filesystem::is_regular_file(status),
                "destination must be a regular file, not a symlink or directory");
        Json document = updated;
        if (std::filesystem::exists(status)) {
            FileResourceManager files(parent);
            const auto existing = files.load(destination.filename());
            const std::string text(existing->bytes.begin(), existing->bytes.end());
            // Do not silently overwrite corrupt or unsupported configurations.
            (void)parse_engine_settings(text, destination.generic_string());
            document = Json::parse(text);
            document["window"] = updated.at("window");
            document["audio"] = updated.at("audio");
        }
        const auto text = document.dump(2) + '\n';
        PendingSettings pending(parent);
        std::ofstream output(pending.file(), std::ios::binary | std::ios::trunc);
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.close();
        require(static_cast<bool>(output), "failed to write or close temporary settings file");
        // Never delete the old file first. If replacement fails, it stays intact.
        std::filesystem::rename(pending.file(), destination);
    } catch (const ConfigurationError& error) {
        throw ConfigurationError("Cannot save settings '" + path.generic_string() + "': " + error.what());
    } catch (const ResourceError& error) {
        throw ConfigurationError("Cannot save settings '" + path.generic_string() + "': " + error.what());
    } catch (const std::filesystem::filesystem_error& error) {
        throw ConfigurationError("Cannot save settings '" + path.generic_string() + "': " + error.what());
    } catch (const Json::exception& error) {
        throw ConfigurationError("Cannot save settings '" + path.generic_string() + "': " + error.what());
    }
}
SettingsLoadResult load_user_settings(const std::filesystem::path& path, const EngineSettings& defaults) {
    (void)serialize_engine_settings(defaults);
    SettingsLoadResult result{defaults, SettingsLoadStatus::failed, {}};
    try {
        require(!path.empty() && path.has_filename(), "a settings filename is required");
        const auto destination = std::filesystem::absolute(path);
        std::error_code error;
        const auto status = std::filesystem::symlink_status(destination, error);
        if (error && error != std::errc::no_such_file_or_directory)
            throw std::filesystem::filesystem_error("Cannot inspect settings", destination, error);
        if (status.type() == std::filesystem::file_type::not_found) {
            // Windows can report ENOENT for "existing-file/settings.json",
            // whereas POSIX reports ENOTDIR. Only a missing path under a
            // directory qualifies as a normal first launch.
            for (auto parent = destination.parent_path(); !parent.empty();) {
                const auto parent_status = std::filesystem::status(parent, error);
                if (error && error != std::errc::no_such_file_or_directory)
                    throw std::filesystem::filesystem_error("Cannot inspect settings parent", parent, error);
                if (std::filesystem::exists(parent_status)) {
                    require(std::filesystem::is_directory(parent_status),
                            "settings parent must be a directory: " + parent.generic_string());
                    break;
                }
                const auto next = parent.parent_path();
                if (next == parent) break;
                parent = next;
            }
            result.status = SettingsLoadStatus::missing;
            return result;
        }
        require(std::filesystem::is_regular_file(status),
                "settings must be a regular file, not a symlink or directory");
        result.settings = load_engine_settings(destination.parent_path(), destination.filename());
        result.status = SettingsLoadStatus::loaded;
        return result;
    } catch (const ConfigurationError& error) {
        result.diagnostic = error.what();
    } catch (const ResourceError& error) {
        result.diagnostic = error.what();
    } catch (const std::filesystem::filesystem_error& error) {
        result.diagnostic = error.what();
    }
    result.diagnostic = "Cannot load user settings '" + path.generic_string() + "': " + result.diagnostic;
    return result;
}
EngineSettings load_engine_settings(const std::filesystem::path& root, const std::filesystem::path& key) {
    FileResourceManager files(root); // Fresh read; no stale configuration cache on reload.
    const auto file = files.load(key);
    const std::string text(file->bytes.begin(), file->bytes.end());
    return parse_engine_settings(text, file->source.generic_string());
}
}
