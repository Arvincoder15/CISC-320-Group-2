#pragma once

#include "engine/ai.hpp"
#include "playable_level.hpp"
#include <span>

namespace prototype {

// Feet coordinates, like players.
inline engine::Aabb acid_bounds(engine::Vec2 feet) {
    return {{feet.x - 16, feet.y - 32}, {32, 32}};
}

// A horizontal slice of walkable, fully supported positions on the spawn's floor.
// Expanding the body by half a cell keeps the entire segment between nodes safe.
// Slimes patrol connected floor. Closed doors and hazards interrupt the route.
class AcidNavigation {
public:
    static constexpr float spacing = 8;
    AcidNavigation(const PlayableLevelDefinition& level, engine::Vec2 home,
                   std::span<const Rect> closed_doors)
        : grid_(static_cast<int>(level.bounds.x / spacing) + 1, 1), floor_(home.y),
          surfaces_(connected_floor_spans(level.platforms, floor_)) {
        for (int x = 0; x < grid_.width(); ++x) {
            const float center = x * spacing;
            const Rect area{center - 20, floor_ - 32, 40, 32};
            bool safe = inside_world(area, level.bounds);
            const bool supported = std::any_of(surfaces_.begin(), surfaces_.end(),
                [&](const auto& surface) {
                    return area.x >= surface.first && area.x + area.w <= surface.second;
                });
            for (const auto& platform : level.platforms) {
                if (engine::overlaps(bounds(area), bounds(platform))) safe = false;
            }
            for (const auto& door : closed_doors)
                if (engine::overlaps(bounds(area), bounds(door))) safe = false;
            for (const auto& hazard : level.hazards)
                if (engine::overlaps(bounds(area), bounds(hazard.box))) safe = false;
            grid_.set_blocked({x, 0}, !safe || !supported);
        }
    }

    engine::GridCell cell(engine::Vec2 p) const {
        return {static_cast<int>(std::round(p.x / spacing)), 0};
    }
    engine::Vec2 point(engine::GridCell c) const { return {c.x * spacing, floor_}; }
    bool shares_floor(engine::Vec2 from, engine::Vec2 target) const {
        return std::abs(target.y - floor_) < 0.02F && next(from, target).has_value();
    }

    std::optional<engine::Vec2> patrol_endpoint(engine::Vec2 from, bool left) const {
        auto edge = cell(from);
        if (!grid_.walkable(edge)) return std::nullopt;
        const int direction = left ? -1 : 1;
        while (grid_.walkable({edge.x + direction, 0})) edge.x += direction;
        return point(edge);
    }

    std::optional<engine::Vec2> next(engine::Vec2 from, engine::Vec2 goal) const {
        const auto surface = std::find_if(surfaces_.begin(), surfaces_.end(),
            [&](const auto& span) { return from.x >= span.first && from.x <= span.second; });
        // Players can stand closer to the edge than a slime. Approach them
        // as far as the slime's clearance permits, but never cross a gap.
        if (surface == surfaces_.end() ||
            goal.x + 14 <= surface->first || goal.x - 14 >= surface->second)
            return std::nullopt;
        const int first = static_cast<int>(std::ceil((surface->first + 20) / spacing));
        const int last = static_cast<int>(std::floor((surface->second - 20) / spacing));
        if (first > last) return std::nullopt;
        const auto goal_cell = engine::GridCell{std::clamp(cell(goal).x, first, last), 0};
        const auto route = engine::find_path(grid_, cell(from), goal_cell);
        if (route.empty()) return std::nullopt;
        // Every node is on the same horizontal line, so the whole path
        // can be followed as one segment at the selected movement speed.
        return point(route.back());
    }
private:
    engine::NavigationGrid grid_;
    float floor_;
    std::vector<std::pair<float, float>> surfaces_;
};

struct AcidJump {
    float landing_x;
    float speed;
    float delay{};
};

struct AcidPatrol {
    AcidNavigation navigation;
    engine::AiController controller;
};

struct AcidEnemy {
    engine::EntityId entity{};
    engine::Vec2 spawn{};
    std::optional<AcidPatrol> patrol{};
    bool facing_left{true};
    float animation_time{};
    std::optional<std::size_t> pursuit_target{};
    std::optional<AcidJump> active_jump{};
    float jump_recovery{};
};

inline constexpr float acid_move_speed = 92;

inline engine::AiConfig acid_ai_config(float world_width) {
    engine::AiConfig config;
    config.detection_range = world_width;
    config.attack_range = 0; // Contact, not the AI's ranged attack event, owns damage.
    config.move_speed = acid_move_speed; // Players can outrun and jump over a slime.
    config.arrival_radius = 2;
    config.search_duration = 1;
    return config;
}

} // namespace prototype
