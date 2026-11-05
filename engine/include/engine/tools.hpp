#pragma once
#include "engine/types.hpp"
#include <string>
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

// Gunveer: JSON/Tiled import and ImGui UI adapt into this neutral model.
std::vector<std::string> validate(const LevelDocument& document);
}
