#include "engine/tools.hpp"
#include <cmath>
namespace engine {
std::vector<std::string> validate(const LevelDocument& document) {
    std::vector<std::string> errors;
    if (document.schema_version != 1) errors.emplace_back("Unsupported level schema version");
    for (const auto& spawn : document.spawns) {
        if (spawn.prefab.empty()) errors.emplace_back("Spawn requires a prefab key");
        if (!std::isfinite(spawn.position.x) || !std::isfinite(spawn.position.y))
            errors.emplace_back("Spawn position must be finite");
    }
    return errors;
}
}
