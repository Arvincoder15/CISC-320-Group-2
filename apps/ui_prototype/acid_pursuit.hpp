#pragma once

#include "acid_enemy.hpp"
#include <limits>
#include <map>
#include <queue>

namespace prototype {

inline constexpr float acid_jump_velocity = -630.0F;
inline constexpr float acid_air_speed = 220.0F;
// Hunters walk immediately. These delays give players time to operate a
// switch and leave a ledge before the next jump arrives.
inline constexpr float hunter_first_jump_delay = 4.6F;
inline constexpr float hunter_landing_recovery = 1.4F;

struct AcidRouteStep {
    float x;
    std::optional<AcidJump> jump;
};

// Nodes are positions where the slime can stand. Walking and ballistic jumps
// are separate directed edges so climbability depends on height and clearance.
class AcidPursuit {
public:
    AcidPursuit(const PlayableLevelDefinition& level, std::span<const Rect> closed_doors)
        : world_size_(level.bounds), obstacles_(level.platforms) {
        obstacles_.insert(obstacles_.end(), closed_doors.begin(), closed_doors.end());
        for (const auto& hazard : level.hazards) obstacles_.push_back(hazard.box);
        for (const auto& platform : level.platforms) {
            if (!floors_.contains(platform.y))
                floors_.emplace(platform.y, connected_floor_spans(level.platforms, platform.y));
        }
        for (const auto& [y, spans] : floors_) {
            for (const auto& [left, right] : spans) {
                // Physics only requires some overlap with the platform. Allow
                // the slime to overhang, with a margin for small frame variations.
                const float first = std::max(16.0F, left - 8);
                const float last = std::min(world_size_.x - 16, right + 8);
                if (first > last) continue;
                const int segments = std::max(2, static_cast<int>(std::ceil((last - first) / 16)));
                for (int i = 0; i <= segments; ++i)
                    add_node({std::lerp(first, last, static_cast<float>(i) / segments), y});
            }
        }

        for (auto& origin : nodes_) {
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                const auto goal = nodes_[i].point;
                const float dx = std::abs(goal.x - origin.point.x);
                if (walkable(origin.point, goal)) {
                    if (dx > 0 && dx <= 24) origin.edges.push_back({i, dx / acid_move_speed, {}});
                } else if (const auto jump = jump_to(origin.point, goal)) {
                    origin.edges.push_back({i, jump->seconds + 0.12F,
                                            AcidJump{goal.x, jump->speed, jump->delay}});
                }
            }
        }
    }

    std::optional<AcidRouteStep> next(engine::Vec2 from, engine::Vec2 goal) const {
        if (walkable(from, goal)) return AcidRouteStep{goal.x, {}};
        std::size_t start = nodes_.size();
        float closest = std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            const float dx = std::abs(nodes_[i].point.x - from.x);
            if (dx < closest && walkable(from, nodes_[i].point)) {
                closest = dx;
                start = i;
            }
        }
        if (start == nodes_.size()) return std::nullopt;

        // Find the best reachable approach, including when a closed gate or an
        // airborne player means there is no standing node at the exact target.
        using Entry = std::pair<float, std::size_t>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<>> queue;
        std::vector<float> cost(nodes_.size(), std::numeric_limits<float>::infinity());
        std::vector<const Edge*> first_step(nodes_.size());
        cost[start] = 0;
        queue.emplace(0, start);
        std::size_t best = start;
        float best_distance = distance(nodes_[start].point, goal);
        while (!queue.empty()) {
            const auto [seconds, current] = queue.top();
            queue.pop();
            if (seconds > cost[current]) continue;
            const auto separation = distance(nodes_[current].point, goal);
            if (separation < best_distance) {
                best = current;
                best_distance = separation;
            }
            for (const auto& edge : nodes_[current].edges) {
                const float next_cost = seconds + edge.seconds;
                if (next_cost >= cost[edge.to]) continue;
                cost[edge.to] = next_cost;
                first_step[edge.to] = current == start ? &edge : first_step[current];
                queue.emplace(next_cost, edge.to);
            }
        }
        if (best == start) return AcidRouteStep{nodes_[start].point.x, {}};
        const auto* edge = first_step[best];
        if (edge->jump && closest > 0.05F) return AcidRouteStep{nodes_[start].point.x, {}};
        return AcidRouteStep{nodes_[edge->to].point.x, edge->jump};
    }

private:
    struct Edge {
        std::size_t to;
        float seconds;
        std::optional<AcidJump> jump;
    };
    struct Node {
        engine::Vec2 point;
        std::vector<Edge> edges;
    };
    struct JumpArc {
        float seconds;
        float speed;
        float delay;
    };

    static float distance(engine::Vec2 a, engine::Vec2 b) {
        return std::hypot(a.x - b.x, a.y - b.y);
    }

    bool clear(engine::Aabb body) const {
        return body.position.x >= 0 && body.position.x + body.size.x <= world_size_.x &&
            body.position.y + body.size.y <= world_size_.y &&
            std::none_of(obstacles_.begin(), obstacles_.end(), [&](const Rect& obstacle) {
                return engine::overlaps(body, bounds(obstacle));
            });
    }

    void add_node(engine::Vec2 point) {
        if (clear(acid_bounds(point))) nodes_.push_back({point, {}});
    }

    bool walkable(engine::Vec2 from, engine::Vec2 goal) const {
        if (std::abs(from.y - goal.y) > 0.02F) return false;
        const float left = std::min(from.x, goal.x);
        const float right = std::max(from.x, goal.x);
        // Nodes can straddle a seam between two touching floor tiles.
        const auto floor = floors_.lower_bound(from.y - 0.02F);
        if (floor == floors_.end() || floor->first > from.y + 0.02F) return false;
        const auto& spans = floor->second;
        return std::any_of(spans.begin(), spans.end(), [&](const auto& span) {
            return left + 16 > span.first && right - 16 < span.second;
        }) && clear({{left - 16, from.y - 32}, {right - left + 32, 32}});
    }

    std::optional<JumpArc> jump_to(engine::Vec2 from, engine::Vec2 goal) const {
        constexpr float gravity = 1000;
        // Use the shorter arc produced by semi-implicit Euler at 60 Hz so
        // there is time to reach the landing point at normal frame rates.
        constexpr float v = acid_jump_velocity + gravity / 120;
        const float dy = goal.y - from.y;
        const float dx = goal.x - from.x;
        const float discriminant = v * v + 2 * gravity * dy;
        if (discriminant <= 0 || std::abs(dx) > 350) return {};
        const float seconds = (-v + std::sqrt(discriminant)) / gravity;
        if (std::abs(dx) > acid_air_speed * (seconds - 0.025F)) return {};
        const int samples = static_cast<int>(std::ceil(seconds * 120));
        // A narrow ledge may require a mostly vertical takeoff from the side
        // before steering over its top, so check delayed horizontal movement too.
        for (float delay : {0.0F, 0.08F, 0.16F, 0.24F, 0.32F, 0.40F}) {
            const float travel_time = seconds - delay - 0.025F;
            if (travel_time <= 0) break;
            const float speed = std::abs(dx) / travel_time;
            if (speed > acid_air_speed) break;
            bool unobstructed = true;
            for (int i = 1; i < samples; ++i) {
                const float t = seconds * i / samples;
                const float x = from.x + std::copysign(
                    std::min(std::abs(dx), speed * std::max(0.0F, t - delay)), dx);
                const float y = from.y + (v + gravity * t / 2) * t;
                if (!clear(acid_bounds({x, y}))) {
                    unobstructed = false;
                    break;
                }
            }
            if (unobstructed) return JumpArc{seconds, speed, delay};
        }
        return {};
    }

    engine::Vec2 world_size_;
    std::vector<Rect> obstacles_;
    std::vector<Node> nodes_;
    std::map<float, std::vector<std::pair<float, float>>> floors_;
};

} // namespace prototype
