#include "engine/ai.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace engine {
namespace {

// Validate at API boundaries so bad values cannot poison persistent AI state.
void require_position(Vec2 value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y))
        throw std::invalid_argument("AI positions must be finite");
}

void require_nonnegative(float value) {
    if (!std::isfinite(value) || value < 0)
        throw std::invalid_argument("AI distances, speeds and times must be finite and nonnegative");
}

// Double intermediates avoid overflow when subtracting valid large float positions.
double distance(Vec2 a, Vec2 b) {
    return std::hypot(static_cast<double>(a.x) - b.x, static_cast<double>(a.y) - b.y);
}

void validate_route(std::span<const Vec2> route) {
    for (const auto point : route) require_position(point);
}

bool inside(const NavigationGrid& grid, GridCell cell) {
    return cell.x >= 0 && cell.y >= 0 && cell.x < grid.width() && cell.y < grid.height();
}

} // namespace

float PatrolController::direction(float position_x, float left_bound, float right_bound) {
    if (!std::isfinite(position_x) || !std::isfinite(left_bound) || !std::isfinite(right_bound)
        || left_bound >= right_bound)
        throw std::invalid_argument("Patrol requires a finite position and ordered bounds");
    // Between the bounds, keep the direction chosen on the previous call.
    if (position_x >= right_bound) direction_ = -1;
    else if (position_x <= left_bound) direction_ = 1;
    return direction_;
}

NavigationGrid::NavigationGrid(int width, int height) : width_(width), height_(height) {
    // Limit cell count so integer indexes and path costs remain representable.
    if (width <= 0 || height <= 0 || width > std::numeric_limits<int>::max() / height)
        throw std::invalid_argument("Navigation grid dimensions are invalid or too large");
    blocked_.resize(static_cast<std::size_t>(width) * height, false);
}

bool NavigationGrid::walkable(GridCell cell) const {
    return inside(*this, cell) && !blocked_[static_cast<std::size_t>(cell.y) * width_ + cell.x];
}

void NavigationGrid::set_blocked(GridCell cell, bool blocked) {
    if (!inside(*this, cell)) throw std::invalid_argument("Obstacle cell is outside the grid");
    blocked_[static_cast<std::size_t>(cell.y) * width_ + cell.x] = blocked;
}

std::vector<GridCell> find_path(const NavigationGrid& grid, GridCell start, GridCell goal) {
    if (!grid.walkable(start) || !grid.walkable(goal)) return {};

    // A* expands the smallest g+h score. Manhattan distance is an admissible
    // estimate for four-direction, unit-cost movement, so the result is shortest.
    const auto index = [&grid](GridCell cell) { return cell.y * grid.width() + cell.x; };
    const auto cell_at = [&grid](int id) { return GridCell{id % grid.width(), id / grid.width()}; };
    const auto heuristic = [goal](GridCell cell) {
        return std::abs(cell.x - goal.x) + std::abs(cell.y - goal.y);
    };
    struct Entry { std::int64_t score; int cost; int id; };
    const auto worse = [](const Entry& a, const Entry& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.id > b.id; // Stable tie-break gives reproducible paths.
    };
    std::priority_queue<Entry, std::vector<Entry>, decltype(worse)> open(worse);
    const auto count = static_cast<std::size_t>(grid.width()) * grid.height();
    std::vector<int> costs(count, std::numeric_limits<int>::max());
    std::vector<int> parents(count, -1);
    const int start_id = index(start);
    const int goal_id = index(goal);
    costs[start_id] = 0;
    open.push({heuristic(start), 0, start_id});
    constexpr std::array<GridCell, 4> offsets{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};

    while (!open.empty()) {
        const Entry current = open.top();
        open.pop();
        if (current.cost != costs[current.id]) continue; // Discard superseded queue entries.
        if (current.id == goal_id) {
            // Parent links run backwards; reverse them into start-to-goal order.
            std::vector<GridCell> path;
            for (int id = goal_id; id != -1; id = parents[id]) path.push_back(cell_at(id));
            std::reverse(path.begin(), path.end());
            return path;
        }
        const GridCell cell = cell_at(current.id);
        for (const auto offset : offsets) {
            const GridCell next{cell.x + offset.x, cell.y + offset.y};
            if (!grid.walkable(next)) continue;
            const int next_id = index(next);
            const int cost = current.cost + 1;
            if (cost >= costs[next_id]) continue;
            costs[next_id] = cost;
            parents[next_id] = current.id;
            open.push({static_cast<std::int64_t>(cost) + heuristic(next), cost, next_id});
        }
    }
    return {};
}

bool has_line_of_sight(const NavigationGrid& grid, GridCell from, GridCell to) {
    if (!grid.walkable(from) || !grid.walkable(to)) return false;

    // Supercover traversal checks every tile touched by the centre-to-centre ray.
    // Integer comparisons avoid floating-point ambiguity at exact tile corners.
    const std::int64_t dx = std::abs(to.x - from.x);
    const std::int64_t dy = std::abs(to.y - from.y);
    const int step_x = to.x > from.x ? 1 : -1;
    const int step_y = to.y > from.y ? 1 : -1;
    std::int64_t crossed_x = 0;
    std::int64_t crossed_y = 0;
    while (from != to) {
        const auto horizontal = (2 * crossed_x + 1) * dy;
        const auto vertical = (2 * crossed_y + 1) * dx;
        if (horizontal == vertical) {
            if (!grid.walkable({from.x + step_x, from.y})
                || !grid.walkable({from.x, from.y + step_y})) return false;
            from.x += step_x;
            from.y += step_y;
            ++crossed_x;
            ++crossed_y;
        } else if (horizontal < vertical) {
            from.x += step_x;
            ++crossed_x;
        } else {
            from.y += step_y;
            ++crossed_y;
        }
        if (!grid.walkable(from)) return false;
    }
    return true;
}

WaypointFollower::WaypointFollower(std::vector<Vec2> points, float arrival_radius)
    : points_(std::move(points)), arrival_radius_(arrival_radius) {
    validate_route(points_);
    require_nonnegative(arrival_radius_);
}

std::optional<Vec2> WaypointFollower::update(Vec2 position) {
    require_position(position);
    // A loop handles repeated points and several waypoints within the arrival radius.
    while (!finished() && distance(position, points_[next_]) <= arrival_radius_) ++next_;
    if (finished()) return std::nullopt;
    return points_[next_];
}

AiController::AiController(AiConfig config, std::vector<Vec2> patrol_route)
    : config_(config), patrol_route_(std::move(patrol_route)) {
    for (float value : {config.detection_range, config.attack_range, config.move_speed,
                       config.arrival_radius, config.attack_interval,
                       config.search_duration, config.stun_duration}) require_nonnegative(value);
    if (config.attack_range > config.detection_range || config.attack_interval == 0)
        throw std::invalid_argument("Attack range must fit detection range; interval must be positive");
    validate_route(patrol_route_);
}

void AiController::reset() {
    patrol_index_ = 0;
    state_ = AiState::patrol;
    last_seen_.reset();
    search_remaining_ = 0;
    attack_remaining_ = 0;
    stun_remaining_ = 0;
}

AiDecision AiController::update(const AiObservation& observation, float dt) {
    require_position(observation.position);
    require_nonnegative(dt);
    for (const auto& target : observation.targets) {
        require_position(target.position);
        if (target.entity == invalid_entity) throw std::invalid_argument("AI target ID is invalid");
    }

    // Tick timers once. A damage event interrupts movement/attacks immediately.
    attack_remaining_ = std::max(0.0F, attack_remaining_ - dt);
    stun_remaining_ = std::max(0.0F, stun_remaining_ - dt);
    if (observation.took_damage) stun_remaining_ = config_.stun_duration;
    if (stun_remaining_ > 0) {
        state_ = AiState::stunned;
        return {state_};
    }

    // Select the nearest visible eligible target, breaking equal-distance ties by ID.
    // Gameplay filters teams, dead actors and the agent itself before this call.
    const AiTarget* selected = nullptr;
    double nearest = config_.detection_range;
    if (config_.engage_targets) {
        for (const auto& target : observation.targets) {
            const double separation = distance(observation.position, target.position);
            if (!target.visible || separation > nearest) continue;
            if (selected && separation == nearest && target.entity >= selected->entity) continue;
            selected = &target;
            nearest = separation;
        }
    }
    if (selected) {
        last_seen_ = selected->position;
        search_remaining_ = config_.search_duration;
        if (nearest <= config_.attack_range) {
            state_ = AiState::attack;
            const bool attack = dt > 0 && attack_remaining_ == 0;
            if (attack) attack_remaining_ = config_.attack_interval;
            return {state_, std::nullopt, selected->entity, 0, attack};
        }
        state_ = AiState::chase;
        return {state_, selected->position, selected->entity, config_.move_speed, false};
    }

    // Loss of sight starts a bounded search at the last observed position.
    // Hidden target positions are never used, and attacks stop as soon as sight is lost.
    if (last_seen_) {
        search_remaining_ = std::max(0.0F, search_remaining_ - dt);
        if (search_remaining_ > 0) {
            state_ = AiState::search;
            if (distance(observation.position, *last_seen_) > config_.arrival_radius)
                return {state_, last_seen_, invalid_entity, config_.move_speed, false};
            return {state_}; // Wait at the last seen location for the remaining search time.
        }
        last_seen_.reset();
    }

    // Cycle through patrol waypoints. Bound the loop so coincident points cannot hang.
    state_ = AiState::patrol;
    for (std::size_t checked = 0; checked < patrol_route_.size(); ++checked) {
        const Vec2 point = patrol_route_[patrol_index_];
        if (distance(observation.position, point) > config_.arrival_radius)
            return {state_, point, invalid_entity, config_.move_speed, false};
        patrol_index_ = (patrol_index_ + 1) % patrol_route_.size();
    }
    return {state_}; // Empty route or all waypoints already reached.
}

} // namespace engine
