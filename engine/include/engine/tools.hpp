#pragma once
#include "engine/types.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace engine {
struct SpawnDefinition {
    std::string prefab;
    Vec2 position{};
    std::string id;
};

struct LevelObject {
    std::string id;
    std::string type;
    std::string prefab;
    Vec2 position{};
    Vec2 size{1.0F, 1.0F};
    std::string parent_id;
    std::string linked_id;
};

struct LevelDocument {
    int schema_version{1};
    std::vector<SpawnDefinition> spawns;
    std::string name;
    Vec2 size{};
    std::vector<LevelObject> objects;
};

class LevelDocumentError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class LevelFileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

std::vector<std::string> validate(const LevelDocument& document);

// Native versioned JSON format; distinct from Tiled JSON and game-specific schemas.
std::string serialize_level_json(const LevelDocument& document);
LevelDocument parse_level_json(std::string_view json, std::string_view source = "<memory>");

// File paths may be absolute or relative to the process working directory.
// Saves require an existing parent directory and replace files through a sibling temporary.
LevelDocument load_level_file(const std::filesystem::path& path);
void save_level_file(const std::filesystem::path& path, const LevelDocument& document);

// Loads and validates a complete replacement before changing the active document.
void reload_level(LevelDocument& active, const std::filesystem::path& path);
}
