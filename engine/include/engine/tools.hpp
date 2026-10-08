#pragma once
#include "engine/types.hpp"
#include <string>
#include <vector>
namespace engine {
struct SpawnDefinition { std::string prefab; Vec2 position{}; };
struct LevelDocument { int schema_version{1}; std::vector<SpawnDefinition> spawns; };
// Gunveer: JSON/Tiled import and ImGui UI will adapt into this neutral model.
std::vector<std::string> validate(const LevelDocument& document);
}
