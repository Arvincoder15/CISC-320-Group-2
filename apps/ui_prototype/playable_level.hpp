#pragma once

#include "engine/physics.hpp"
#include "engine/tools.hpp"
#include "elemental_coop/game.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace prototype {

struct Rect {
    float x{};
    float y{};
    float w{};
    float h{};
};

struct Pool {
    Rect box;
    elemental_coop::Hazard kind{};
};

struct PuzzleGate {
    std::string id;
    Rect door;
    std::string lever_id;
    Rect lever;
    std::string plate_id;
    Rect plate;
    bool require_distinct_operators{};
};

struct ExitDefinition {
    std::string id;
    Rect area;
};

struct PlayableLevelDefinition {
    std::string name;
    bool hard_enemy_pursuit{};
    engine::Vec2 bounds{};
    std::array<engine::Vec2, elemental_coop::player_count> spawns{};
    std::vector<engine::SpawnDefinition> enemy_spawns;
    std::vector<Rect> platforms;
    std::vector<Pool> hazards;
    std::vector<PuzzleGate> gates;
    std::array<ExitDefinition, elemental_coop::player_count> exits{};
};

class PlayableLevelError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

inline engine::Aabb bounds(const Rect& rectangle) {
    return {{rectangle.x, rectangle.y}, {rectangle.w, rectangle.h}};
}

inline bool inside_world(const Rect& rectangle, engine::Vec2 world_size) {
    return rectangle.x >= 0.0F && rectangle.y >= 0.0F &&
        rectangle.w > 0.0F && rectangle.h > 0.0F &&
        rectangle.x + rectangle.w <= world_size.x &&
        rectangle.y + rectangle.h <= world_size.y;
}

// Platforms with flush tops form one floor, even when authored as separate tiles.
inline std::vector<std::pair<float, float>> connected_floor_spans(
    const std::vector<Rect>& platforms, float floor_y) {
    std::vector<std::pair<float, float>> spans;
    for (const auto& platform : platforms) {
        const float right = platform.x + platform.w;
        if (std::abs(platform.y - floor_y) <= 0.01F &&
            std::isfinite(platform.x) && std::isfinite(right) && right > platform.x)
            spans.emplace_back(platform.x, right);
    }
    std::sort(spans.begin(), spans.end());
    std::size_t merged = 0;
    for (const auto& [left, right] : spans) {
        if (merged > 0 && left <= spans[merged - 1].second)
            spans[merged - 1].second = std::max(spans[merged - 1].second, right);
        else
            spans[merged++] = {left, right};
    }
    spans.resize(merged);
    return spans;
}

inline bool shares_player_spawn_floor(const PlayableLevelDefinition& level, engine::Vec2 enemy) {
    for (const auto& [left, right] : connected_floor_spans(level.platforms, enemy.y)) {
        if (enemy.x < left || enemy.x > right) continue;
        for (const auto player : level.spawns) {
            // Player feet may overhang the platform as long as the collider is supported.
            if (std::abs(player.y - enemy.y) <= 0.02F &&
                player.x + 14 > left && player.x - 14 < right)
                return true;
        }
    }
    return false;
}

inline std::string enemy_spawn_floor_error(const engine::SpawnDefinition& spawn) {
    return "Acid enemy spawn '" + spawn.id + "' shares a starting floor with a player";
}

inline std::string join_errors(const std::vector<std::string>& errors) {
    std::ostringstream message;
    for (std::size_t i = 0; i < errors.size(); ++i) {
        if (i != 0) message << "; ";
        message << errors[i];
    }
    return message.str();
}

inline PlayableLevelDefinition playable_level_from_document(const engine::LevelDocument& document) {
    constexpr engine::Vec2 supported_world{1280.0F, 720.0F};
    constexpr engine::Vec2 player_size{28.0F, 46.0F};
    std::vector<std::string> errors = engine::validate(document);
    if (document.name.empty()) errors.emplace_back("Level name must not be empty");
    if (document.size.x != supported_world.x || document.size.y != supported_world.y)
        errors.emplace_back("Playable levels must use 1280 x 720 pixel bounds");

    PlayableLevelDefinition result;
    result.name = document.name;
    result.hard_enemy_pursuit = document.name.ends_with(" - Hard");
    result.bounds = document.size;
    std::set<std::string> all_ids;
    std::array<bool, elemental_coop::player_count> spawn_found{};
    for (const auto& spawn : document.spawns) {
        if (spawn.id.empty() || !all_ids.insert(spawn.id).second)
            errors.emplace_back("Spawn IDs must be nonempty and unique across the document");
        std::size_t role = elemental_coop::player_count;
        if (spawn.prefab == "fire_player") role = 0;
        else if (spawn.prefab == "water_player") role = 1;
        else if (spawn.prefab == "acid_enemy") result.enemy_spawns.push_back(spawn);
        else errors.emplace_back("Unsupported spawn prefab: " + spawn.prefab);
        if (role < elemental_coop::player_count) {
            if (spawn_found[role]) errors.emplace_back("Level has duplicate player role: " + spawn.prefab);
            spawn_found[role] = true;
            result.spawns[role] = spawn.position;
        }
    }
    if (!spawn_found[0] || !spawn_found[1]) {
        errors.emplace_back("Playable levels require exactly one fire_player and one water_player spawn");
    }

    std::unordered_map<std::string, const engine::LevelObject*> objects;
    std::set<std::string> platform_ids;
    std::unordered_map<std::string, std::size_t> gate_indices;
    std::vector<std::pair<const engine::LevelObject*, std::size_t>> levers;
    std::vector<std::pair<const engine::LevelObject*, std::size_t>> plates;
    std::array<bool, elemental_coop::player_count> exit_found{};
    for (const auto& object : document.objects) {
        if (object.id.empty() || !all_ids.insert(object.id).second)
            errors.emplace_back("Object IDs must be nonempty and unique across the document: " + object.id);
        objects.emplace(object.id, &object);
        const Rect rectangle{object.position.x, object.position.y, object.size.x, object.size.y};
        if (!inside_world(rectangle, document.size))
            errors.emplace_back("Object is outside level bounds: " + object.id);

        if (object.type == "platform") {
            if (object.prefab != "solid") errors.emplace_back("Unsupported platform prefab: " + object.prefab);
            if (!object.linked_id.empty())
                errors.emplace_back("Platform linked_id is unsupported: " + object.id);
            platform_ids.insert(object.id);
            result.platforms.push_back(rectangle);
        } else if (object.type == "hazard") {
            if (!object.linked_id.empty())
                errors.emplace_back("Hazard linked_id is unsupported: " + object.id);
            elemental_coop::Hazard kind{};
            if (object.prefab == "lava") kind = elemental_coop::Hazard::lava;
            else if (object.prefab == "water") kind = elemental_coop::Hazard::water;
            else if (object.prefab == "toxic") kind = elemental_coop::Hazard::toxic;
            else {
                errors.emplace_back("Unsupported hazard prefab: " + object.prefab);
                continue;
            }
            result.hazards.push_back({rectangle, kind});
        } else if (object.type == "door") {
            bool distinct = false;
            if (object.prefab == "cooperative_gate_distinct") distinct = true;
            else if (object.prefab != "cooperative_gate")
                errors.emplace_back("Unsupported door prefab: " + object.prefab);
            if (!object.linked_id.empty())
                errors.emplace_back("Door linked_id must be empty; controls link to the door instead: " + object.id);
            gate_indices.emplace(object.id, result.gates.size());
            result.gates.push_back({object.id, rectangle, {}, {}, {}, {}, distinct});
        } else if (object.type == "lever") {
            if (object.prefab != "cooperative_lever")
                errors.emplace_back("Unsupported lever prefab: " + object.prefab);
            levers.emplace_back(&object, result.gates.size());
        } else if (object.type == "pressure_plate") {
            if (object.prefab != "cooperative_plate")
                errors.emplace_back("Unsupported pressure plate prefab: " + object.prefab);
            plates.emplace_back(&object, result.gates.size());
        } else if (object.type == "exit") {
            std::size_t role = elemental_coop::player_count;
            if (object.prefab == "fire_exit") role = 0;
            else if (object.prefab == "water_exit") role = 1;
            else errors.emplace_back("Unsupported exit prefab: " + object.prefab);
            if (role < elemental_coop::player_count) {
                if (exit_found[role]) errors.emplace_back("Level has duplicate matching exit: " + object.prefab);
                exit_found[role] = true;
                result.exits[role] = {object.id, rectangle};
            }
            if (!object.linked_id.empty())
                errors.emplace_back("Exit linked_id is unsupported: " + object.id);
        } else {
            errors.emplace_back("Unsupported playable object type: " + object.type);
        }
    }
    if (result.platforms.empty()) errors.emplace_back("Playable level requires at least one platform");
    if (result.gates.empty() || result.gates.size() > 2)
        errors.emplace_back("Playable levels require one or two cooperative gates");
    if (!exit_found[0] || !exit_found[1] ||
        std::count_if(document.objects.begin(), document.objects.end(),
                      [](const auto& object) { return object.type == "exit"; }) != 2) {
        errors.emplace_back("Playable levels require exactly one fire_exit and one water_exit");
    }

    for (const auto& object : document.objects) {
        if (!object.parent_id.empty()) {
            const auto parent = objects.find(object.parent_id);
            if (parent == objects.end() || parent->second->type != "platform")
                errors.emplace_back("Object parent_id must reference a platform: " + object.id);
        }
    }

    const auto attach_control = [&](const engine::LevelObject* control, bool is_lever) {
        const auto gate = gate_indices.find(control->linked_id);
        if (gate == gate_indices.end()) {
            errors.emplace_back("Control must link to a door: " + control->id);
            return;
        }
        auto& target = result.gates[gate->second];
        auto& control_id = is_lever ? target.lever_id : target.plate_id;
        auto& control_rect = is_lever ? target.lever : target.plate;
        if (!control_id.empty()) {
            errors.emplace_back(std::string("Door has multiple ") +
                                (is_lever ? "levers: " : "pressure plates: ") + target.id);
            return;
        }
        control_id = control->id;
        control_rect = {control->position.x, control->position.y,
                        control->size.x, control->size.y};
        if (control->parent_id.empty() || !platform_ids.contains(control->parent_id))
            errors.emplace_back("Lever and pressure plate require a supporting platform: " + control->id);
    };
    for (const auto& [object, unused] : levers) {
        (void)unused;
        attach_control(object, true);
    }
    for (const auto& [object, unused] : plates) {
        (void)unused;
        attach_control(object, false);
    }
    for (const auto& gate : result.gates) {
        if (gate.lever_id.empty()) errors.emplace_back("Door has no linked lever: " + gate.id);
        if (gate.plate_id.empty()) errors.emplace_back("Door has no linked pressure plate: " + gate.id);
    }
    if (levers.size() != result.gates.size() || plates.size() != result.gates.size())
        errors.emplace_back("Each door must have exactly one lever and one pressure plate");

    for (const auto& object : document.objects) {
        if (object.type != "lever" && object.type != "pressure_plate" && object.type != "exit") continue;
        const auto parent = objects.find(object.parent_id);
        if (parent == objects.end() || parent->second->type != "platform") continue;
        const auto surface_y = parent->second->position.y;
        if (std::abs(object.position.y + object.size.y - surface_y) > 1.0F)
            errors.emplace_back("Control or exit must sit on its parent platform: " + object.id);
        if (object.type == "exit" &&
            (object.position.x < parent->second->position.x ||
             object.position.x + object.size.x > parent->second->position.x + parent->second->size.x)) {
            errors.emplace_back("Exit must be supported by its parent platform: " + object.id);
        }
    }

    if (result.gates.size() == 2) {
        std::sort(result.gates.begin(), result.gates.end(),
                  [](const auto& left, const auto& right) { return left.door.x < right.door.x; });
        const auto& first = result.gates[0];
        const auto& second = result.gates[1];
        if (!first.require_distinct_operators || !second.require_distinct_operators)
            errors.emplace_back("Sequential gates must both require distinct operators");
        if (first.lever.x + first.lever.w > first.door.x ||
            first.plate.x + first.plate.w > first.door.x ||
            second.lever.x < first.door.x + first.door.w ||
            second.plate.x < first.door.x + first.door.w ||
            second.lever.x + second.lever.w > second.door.x ||
            second.plate.x + second.plate.w > second.door.x) {
            errors.emplace_back("Sequential gate controls must be placed on their intended sides");
        }
    }

    for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
        const auto spawn = result.spawns[i];
        const Rect player_box{spawn.x - 14.0F, spawn.y - 46.0F,
                              player_size.x, player_size.y};
        if (!inside_world(player_box, document.size))
            errors.emplace_back("Player spawn collider is outside level bounds");
        const bool supported = std::any_of(result.platforms.begin(), result.platforms.end(),
            [&](const Rect& platform) {
                return std::abs(platform.y - spawn.y) <= 0.01F &&
                    engine::overlaps(bounds(player_box),
                                     {{platform.x, platform.y - 0.01F},
                                      {platform.w, platform.h + 0.01F}});
            });
        if (!supported) errors.emplace_back("Player spawn collider is not supported by a platform");
        for (const auto& hazard : result.hazards) {
            if (engine::overlaps(bounds(player_box), bounds(hazard.box)))
                errors.emplace_back("Player spawn overlaps a hazard");
        }
    }

    for (std::size_t i = 0; i < result.enemy_spawns.size(); ++i) {
        const auto p = result.enemy_spawns[i].position;
        const Rect box{p.x - 16, p.y - 32, 32, 32};
        if (!inside_world(box, document.size)) errors.emplace_back("Acid enemy spawn is outside level bounds");
        const bool supported = std::any_of(result.platforms.begin(), result.platforms.end(),
            [&](const Rect& platform) {
                return std::abs(platform.y - p.y) < 0.01F &&
                    p.x - 20 >= platform.x && p.x + 20 <= platform.x + platform.w;
            });
        if (!supported) errors.emplace_back("Acid enemy spawn needs full platform support and navigation clearance");
        for (const auto& platform : result.platforms)
            if (engine::overlaps(bounds(box), bounds(platform))) errors.emplace_back("Acid enemy spawn overlaps a platform");
        for (const auto& gate : result.gates)
            if (engine::overlaps(bounds(box), bounds(gate.door))) errors.emplace_back("Acid enemy spawn overlaps a door");
        for (const auto& hazard : result.hazards)
            if (engine::overlaps(bounds(box), bounds(hazard.box))) errors.emplace_back("Acid enemy spawn overlaps a hazard");
        if (shares_player_spawn_floor(result, p))
            errors.push_back(enemy_spawn_floor_error(result.enemy_spawns[i]));
        for (const auto player : result.spawns)
            if (std::hypot(player.x - p.x, player.y - p.y) < 80)
                errors.emplace_back("Acid enemy spawn must be at least 80 pixels from players");
        for (std::size_t j = 0; j < i; ++j) {
            const auto other = result.enemy_spawns[j].position;
            if (engine::overlaps(bounds(box), {{other.x - 16, other.y - 32}, {32, 32}}))
                errors.emplace_back("Acid enemy spawns overlap");
        }
    }
    if (!errors.empty()) throw PlayableLevelError(join_errors(errors));
    return result;
}

inline PlayableLevelDefinition load_playable_level_file(const std::filesystem::path& path) {
    return playable_level_from_document(engine::load_level_file(path));
}

} // namespace prototype
