#include "engine/tools.hpp"
#include <cmath>
#include <set>
#include <unordered_map>

namespace engine {
namespace {
bool finite_vec2(const Vec2& value) {
    return std::isfinite(value.x) && std::isfinite(value.y);
}

bool valid_object_reference(const std::string& value, const std::unordered_map<std::string, std::size_t>& object_ids) {
    return value.empty() || object_ids.find(value) != object_ids.end();
}
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

    if (document.size.x < 0.0F || document.size.y < 0.0F) {
        errors.emplace_back("Level size must not be negative");
    }

    return errors;
}
}
