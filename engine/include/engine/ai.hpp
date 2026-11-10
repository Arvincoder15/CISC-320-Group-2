#pragma once
#include "engine/types.hpp"
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace engine {

// Existing left/right patrol API, retained for callers using horizontal bounds.
class PatrolController {
public:
    float direction(float position_x, float left_bound, float right_bound);
private:
    float direction_{1};
};

// Grid coordinates are separate from world-space pixels. Callers own the mapping.
struct GridCell {
    int x{};
    int y{};
    bool operator==(const GridCell&) const = default;
};

// Owned, editable obstacle grid. Invalid dimensions/cell edits throw.
// Queries outside the map return false, making the map edge an obstacle.
class NavigationGrid {
public:
    NavigationGrid(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }
    bool walkable(GridCell cell) const;
    void set_blocked(GridCell cell, bool blocked = true);
private:
    int width_;
    int height_;
    std::vector<bool> blocked_;
};

// Shortest four-neighbour route with unit step costs. Includes both endpoints.
// Empty means blocked/out-of-bounds endpoints or no route. start == goal gives one cell.
std::vector<GridCell> find_path(const NavigationGrid& grid, GridCell start, GridCell goal);

// Tests the segment between cell centres. Every touched cell must be walkable,
// including both cells beside an exact diagonal corner. Suitable for solid tiles.
bool has_line_of_sight(const NavigationGrid& grid, GridCell from, GridCell to);

// Owns a route in world coordinates. update skips reached waypoints and returns
// the next destination. The caller applies velocity/collision and replans as needed.
class WaypointFollower {
public:
    explicit WaypointFollower(std::vector<Vec2> points, float arrival_radius = 1.0F);
    std::optional<Vec2> update(Vec2 position);
    bool finished() const { return next_ == points_.size(); }
private:
    std::vector<Vec2> points_;
    float arrival_radius_;
    std::size_t next_{};
};

// Gameplay supplies eligible targets and visibility from grid LOS or a physics ray.
// This value snapshot avoids storing pointers to entities that may be destroyed.
struct AiTarget {
    EntityId entity{invalid_entity};
    Vec2 position{};
    bool visible{true};
};

enum class AiState { patrol, chase, attack, search, stunned };

// Different NPC/enemy types can use different configurations and patrol routes.
// Set engage_targets=false for a peaceful patrolling NPC. Units are pixels/seconds.
struct AiConfig {
    float detection_range{200.0F};
    float attack_range{20.0F};
    float move_speed{60.0F};
    float arrival_radius{2.0F};
    float attack_interval{1.0F};
    float search_duration{3.0F};
    float stun_duration{0.5F};
    bool engage_targets{true};
};

struct AiObservation {
    Vec2 position{};
    std::span<const AiTarget> targets{};
    bool took_damage{false}; // One-tick event; repeated events restart stun.
};

// A movement destination is an intention for a navigation/movement adapter.
// attack_requested is at most one event per positive-dt update; gameplay owns damage.
struct AiDecision {
    AiState state{AiState::patrol};
    std::optional<Vec2> destination{};
    EntityId target{invalid_entity};
    float move_speed{};
    bool attack_requested{};
};

// One controller per agent. Owns its route and state; borrows observations only
// during update. Invalid config/observations/dt throw before changing state.
class AiController {
public:
    explicit AiController(AiConfig config = {}, std::vector<Vec2> patrol_route = {});
    AiDecision update(const AiObservation& observation, float dt);
    AiState state() const { return state_; }
    void reset(); // Clear memory/timers and restart patrol at its first waypoint.
private:
    AiConfig config_;
    std::vector<Vec2> patrol_route_;
    std::size_t patrol_index_{};
    AiState state_{AiState::patrol};
    std::optional<Vec2> last_seen_{};
    float search_remaining_{};
    float attack_remaining_{};
    float stun_remaining_{};
};

} // namespace engine
