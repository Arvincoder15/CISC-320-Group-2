#include "engine/tools.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <set>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace engine {
namespace {
using Json = nlohmann::json;

bool finite_vec2(const Vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool valid_object_reference(const std::string& value, const std::unordered_map<std::string, std::size_t>& object_ids) {
    return value.empty() || object_ids.find(value) != object_ids.end();
}

void require_document(bool valid, const std::string& message) {
    if (!valid) {
        throw LevelDocumentError(message);
    }
}

const Json& required_field(const Json& object, const char* key, const std::string& context) {
    require_document(object.is_object(), context + " must be an object");
    require_document(object.contains(key), context + " is missing '" + key + "'");
    return object.at(key);
}

void require_known_fields(const Json& object, std::initializer_list<const char*> allowed, const std::string& context) {
    require_document(object.is_object(), context + " must be an object");
    for (const auto& [key, value] : object.items()) {
        (void)value;
        bool known = false;
        for (const auto* field : allowed) {
            if (key == field) {
                known = true;
                break;
            }
        }
        require_document(known, context + " has unsupported field '" + key + "'");
    }
}

std::string required_string(const Json& object, const char* key, const std::string& context) {
    const auto& value = required_field(object, key, context);
    require_document(value.is_string(), context + "." + key + " must be a string");
    return value.get<std::string>();
}

float required_float(const Json& value, const std::string& context) {
    require_document(value.is_number(), context + " must be a number");
    const auto number = value.get<double>();
    require_document(std::isfinite(number) &&
                         number >= -std::numeric_limits<float>::max() &&
                         number <= std::numeric_limits<float>::max(),
                     context + " must be finite and representable");
    return static_cast<float>(number);
}

Vec2 parse_vec2(const Json& value, const std::string& context) {
    require_known_fields(value, {"x", "y"}, context);
    return {
        required_float(required_field(value, "x", context), context + ".x"),
        required_float(required_field(value, "y", context), context + ".y")
    };
}

void require_level_path(const std::filesystem::path& path) {
    if (path.empty() || !path.has_filename()) {
        throw LevelFileError("A level filename is required");
    }
}

Json to_json(const Vec2& value) {
    return Json{{"x", value.x}, {"y", value.y}};
}

std::string read_file_text(const std::filesystem::path& path) {
    require_level_path(path);

    std::error_code error;
    const auto absolute_path = std::filesystem::absolute(path, error);
    if (error) {
        throw LevelFileError("Cannot resolve level file '" + path.generic_string() + "': " + error.message());
    }

    const auto status = std::filesystem::symlink_status(absolute_path, error);
    if (error == std::errc::no_such_file_or_directory || status.type() == std::filesystem::file_type::not_found) {
        throw LevelFileError("Level file '" + absolute_path.generic_string() + "' does not exist");
    }
    if (error) {
        throw LevelFileError("Cannot inspect level file '" + absolute_path.generic_string() + "': " + error.message());
    }
    if (std::filesystem::is_symlink(status) || !std::filesystem::is_regular_file(status)) {
        throw LevelFileError("Level path '" + absolute_path.generic_string() + "' must be a regular non-symlink file");
    }

    std::ifstream input(absolute_path, std::ios::binary);
    if (!input) {
        throw LevelFileError("Cannot open level file '" + absolute_path.generic_string() + "' for reading");
    }
    std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (input.bad()) {
        throw LevelFileError("Failed while reading level file '" + absolute_path.generic_string() + "'");
    }
    return text;
}

class PendingLevelFile {
public:
    explicit PendingLevelFile(const std::filesystem::path& parent) {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            const auto candidate = parent / (".level-save-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            std::error_code error;
            if (std::filesystem::create_directory(candidate, error)) {
                directory_ = candidate;
                return;
            }
            if (error && error != std::errc::file_exists) {
                throw std::filesystem::filesystem_error("Cannot create temporary level directory", candidate, error);
            }
        }
        throw LevelFileError("Unable to create a unique temporary directory for level save");
    }

    ~PendingLevelFile() {
        std::error_code ignored;
        std::filesystem::remove_all(directory_, ignored);
    }

    PendingLevelFile(const PendingLevelFile&) = delete;
    PendingLevelFile& operator=(const PendingLevelFile&) = delete;

    std::filesystem::path file() const {
        return directory_ / "level.json";
    }

private:
    std::filesystem::path directory_;
};
} // namespace

std::vector<std::string> validate(const LevelDocument& document) {
    std::vector<std::string> errors;
    if (document.schema_version != 1) {
        errors.emplace_back("Unsupported level schema version");
    }

    std::set<std::string> seen_spawn_ids;
    for (const auto& spawn : document.spawns) {
        if (spawn.prefab.empty()) {
            errors.emplace_back("Spawn requires a prefab key");
        }
        if (!spawn.id.empty() && !seen_spawn_ids.insert(spawn.id).second) {
            errors.emplace_back("Duplicate spawn id: " + spawn.id);
        }
        if (!finite_vec2(spawn.position)) {
            errors.emplace_back("Spawn position must be finite");
        }
    }

    std::unordered_map<std::string, std::size_t> object_ids;
    std::set<std::string> seen_object_ids;
    for (std::size_t i = 0; i < document.objects.size(); ++i) {
        const auto& object = document.objects[i];
        if (object.id.empty()) {
            errors.emplace_back("Object at index " + std::to_string(i) + " is missing an id");
        } else if (!seen_object_ids.insert(object.id).second) {
            errors.emplace_back("Duplicate object id: " + object.id);
        }
        if (object.type.empty()) {
            errors.emplace_back("Object " + object.id + " is missing a type");
        }
        if (!object.prefab.empty() && object.type.empty()) {
            errors.emplace_back("Object " + object.id + " has a prefab but no type");
        }
        if (!finite_vec2(object.position)) {
            errors.emplace_back("Object " + object.id + " position must be finite");
        }
        if (!finite_vec2(object.size) || object.size.x <= 0.0F || object.size.y <= 0.0F) {
            errors.emplace_back("Object " + object.id + " size must be a positive finite rectangle");
        }
        if (!object.id.empty()) {
            object_ids.emplace(object.id, i);
        }
    }

    for (const auto& object : document.objects) {
        if (!valid_object_reference(object.parent_id, object_ids)) {
            errors.emplace_back("Object " + object.id + " references missing parent id: " + object.parent_id);
        }
        if (!valid_object_reference(object.linked_id, object_ids)) {
            errors.emplace_back("Object " + object.id + " references missing linked id: " + object.linked_id);
        }
    }

    if (!finite_vec2(document.size) || document.size.x < 0.0F || document.size.y < 0.0F) {
        errors.emplace_back("Level size must be finite and nonnegative");
    }

    return errors;
}

std::string serialize_level_json(const LevelDocument& document) {
    const auto errors = validate(document);
    if (!errors.empty()) {
        std::string message = "Cannot serialize invalid level";
        for (const auto& error : errors) {
            message += "; " + error;
        }
        throw LevelDocumentError(message);
    }

    Json spawns = Json::array();
    for (const auto& spawn : document.spawns) {
        spawns.push_back({
            {"id", spawn.id},
            {"prefab", spawn.prefab},
            {"position", to_json(spawn.position)}
        });
    }

    Json objects = Json::array();
    for (const auto& object : document.objects) {
        objects.push_back({
            {"id", object.id},
            {"type", object.type},
            {"prefab", object.prefab},
            {"position", to_json(object.position)},
            {"size", to_json(object.size)},
            {"parent_id", object.parent_id},
            {"linked_id", object.linked_id}
        });
    }

    const Json native_document = {
        {"format", "cisc320.level-document"},
        {"schema_version", document.schema_version},
        {"name", document.name},
        {"size", to_json(document.size)},
        {"spawns", std::move(spawns)},
        {"objects", std::move(objects)}
    };
    return native_document.dump(2) + '\n';
}

LevelDocument parse_level_json(std::string_view text, std::string_view source) {
    try {
        const auto root = Json::parse(text.begin(), text.end());
        require_known_fields(root, {"format", "schema_version", "name", "size", "spawns", "objects"}, "Level document");
        require_document(required_string(root, "format", "Level document") == "cisc320.level-document",
                         "Level document format must be 'cisc320.level-document'");

        const auto& version = required_field(root, "schema_version", "Level document");
        require_document(version.is_number_integer() && version == 1, "Level document schema_version must be integer 1");

        LevelDocument document;
        document.schema_version = version.get<int>();
        document.name = required_string(root, "name", "Level document");
        document.size = parse_vec2(required_field(root, "size", "Level document"), "Level document.size");

        const auto& spawns = required_field(root, "spawns", "Level document");
        require_document(spawns.is_array(), "Level document.spawns must be an array");
        for (std::size_t i = 0; i < spawns.size(); ++i) {
            const auto& value = spawns[i];
            const auto context = "Level document.spawns[" + std::to_string(i) + "]";
            require_known_fields(value, {"id", "prefab", "position"}, context);
            document.spawns.push_back({
                required_string(value, "prefab", context),
                parse_vec2(required_field(value, "position", context), context + ".position"),
                required_string(value, "id", context)
            });
        }

        const auto& objects = required_field(root, "objects", "Level document");
        require_document(objects.is_array(), "Level document.objects must be an array");
        for (std::size_t i = 0; i < objects.size(); ++i) {
            const auto& value = objects[i];
            const auto context = "Level document.objects[" + std::to_string(i) + "]";
            require_known_fields(value, {"id", "type", "prefab", "position", "size", "parent_id", "linked_id"}, context);
            document.objects.push_back({
                required_string(value, "id", context),
                required_string(value, "type", context),
                required_string(value, "prefab", context),
                parse_vec2(required_field(value, "position", context), context + ".position"),
                parse_vec2(required_field(value, "size", context), context + ".size"),
                required_string(value, "parent_id", context),
                required_string(value, "linked_id", context)
            });
        }

        const auto errors = validate(document);
        if (!errors.empty()) {
            std::string message = "Level document failed validation";
            for (const auto& error : errors) {
                message += "; " + error;
            }
            throw LevelDocumentError(message);
        }
        return document;
    } catch (const Json::exception& error) {
        throw LevelDocumentError("Level document '" + std::string(source) + "': invalid JSON or field value: " + error.what());
    } catch (const LevelDocumentError& error) {
        throw LevelDocumentError("Level document '" + std::string(source) + "': " + error.what());
    }
}

LevelDocument load_level_file(const std::filesystem::path& path) {
    try {
        const auto text = read_file_text(path);
        return parse_level_json(text, path.generic_string());
    } catch (const std::filesystem::filesystem_error& error) {
        throw LevelFileError("Cannot load level file '" + path.generic_string() + "': " + error.what());
    }
}

void save_level_file(const std::filesystem::path& path, const LevelDocument& document) {
    require_level_path(path);
    const auto text = serialize_level_json(document);
    try {
        std::error_code error;
        const auto destination = std::filesystem::absolute(path, error);
        if (error) {
            throw std::filesystem::filesystem_error("Cannot resolve level destination", path, error);
        }
        const auto parent = destination.parent_path();
        const auto parent_status = std::filesystem::status(parent, error);
        if (error) {
            throw std::filesystem::filesystem_error("Cannot inspect level destination directory", parent, error);
        }
        if (!std::filesystem::is_directory(parent_status)) {
            throw LevelFileError("Level destination parent '" + parent.generic_string() + "' must be an existing directory");
        }

        const auto destination_status = std::filesystem::symlink_status(destination, error);
        if (error && error != std::errc::no_such_file_or_directory) {
            throw std::filesystem::filesystem_error("Cannot inspect level destination", destination, error);
        }
        if (!error && destination_status.type() != std::filesystem::file_type::not_found) {
            if (std::filesystem::is_symlink(destination_status) || !std::filesystem::is_regular_file(destination_status)) {
                throw LevelFileError("Level destination '" + destination.generic_string() + "' must be a regular non-symlink file");
            }
        }

        PendingLevelFile pending(parent);
        std::ofstream output(pending.file(), std::ios::binary | std::ios::trunc);
        if (!output) {
            throw LevelFileError("Cannot open temporary level file for writing: " + pending.file().generic_string());
        }
        output.write(text.data(), static_cast<std::streamsize>(text.size()));
        output.close();
        if (!output) {
            throw LevelFileError("Failed to write or close temporary level file: " + pending.file().generic_string());
        }

        std::filesystem::rename(pending.file(), destination, error);
        if (error) {
            throw std::filesystem::filesystem_error("Cannot replace level file", pending.file(), destination, error);
        }
    } catch (const LevelFileError&) {
        throw;
    } catch (const std::filesystem::filesystem_error& error) {
        throw LevelFileError("Cannot save level file '" + path.generic_string() + "': " + error.what());
    }
}

void reload_level(LevelDocument& active, const std::filesystem::path& path) {
    static_assert(std::is_nothrow_move_assignable_v<LevelDocument>);
    auto replacement = load_level_file(path);
    active = std::move(replacement);
}
}
