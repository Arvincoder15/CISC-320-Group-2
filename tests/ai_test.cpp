#include "engine/ai.hpp"
#include "engine/architecture.hpp"
#include "engine/physics.hpp"
#include "test_support.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <queue>

namespace {
using namespace engine;

void test_original_patrol() {
    // Protect the original contract used by existing callers.
    PatrolController patrol;
    CHECK(patrol.direction(5, 0, 10) == 1);
    CHECK(patrol.direction(10, 0, 10) == -1);
    CHECK(patrol.direction(5, 0, 10) == -1);
    CHECK(patrol.direction(0, 0, 10) == 1);
    CHECK(throws_as<std::invalid_argument>([&] { patrol.direction(0, 1, 1); }));
    CHECK(throws_as<std::invalid_argument>([&] {
        patrol.direction(std::numeric_limits<float>::infinity(), 0, 1);
    }));
}

// Independent breadth-first search supplies a shortest-distance oracle for A*.
int breadth_first_distance(const NavigationGrid& grid, GridCell start, GridCell goal) {
    if (!grid.walkable(start) || !grid.walkable(goal)) return -1;
    std::vector<int> distances(static_cast<std::size_t>(grid.width()) * grid.height(), -1);
    const auto index = [&grid](GridCell p) { return p.y * grid.width() + p.x; };
    std::queue<GridCell> pending;
    pending.push(start);
    distances[index(start)] = 0;
    constexpr std::array<GridCell, 4> offsets{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
    while (!pending.empty()) {
        const auto current = pending.front();
        pending.pop();
        if (current == goal) return distances[index(current)];
        for (const auto offset : offsets) {
            const GridCell next{current.x + offset.x, current.y + offset.y};
            if (!grid.walkable(next) || distances[index(next)] != -1) continue;
            distances[index(next)] = distances[index(current)] + 1;
            pending.push(next);
        }
    }
    return -1;
}

void test_navigation() {
    CHECK(throws_as<std::invalid_argument>([] { NavigationGrid grid(0, 2); }));
    CHECK(throws_as<std::invalid_argument>([] {
        NavigationGrid grid(std::numeric_limits<int>::max(), 2);
    }));
    NavigationGrid grid(5, 5);
    CHECK(throws_as<std::invalid_argument>([&] { grid.set_blocked({5, 0}); }));
    CHECK(find_path(grid, {-1, 0}, {4, 4}).empty());
    CHECK(find_path(grid, {2, 2}, {2, 2}).size() == 1);
    grid.set_blocked({2, 2});
    CHECK(find_path(grid, {2, 2}, {2, 2}).empty());

    // Enumerate every arrangement of eight obstacle cells on a small map.
    // Verify both optimal length and every step, including unreachable layouts.
    for (unsigned mask = 0; mask < 256; ++mask) {
        NavigationGrid map(4, 4);
        for (unsigned bit = 0; bit < 8; ++bit)
            map.set_blocked({static_cast<int>(bit % 4), 1 + static_cast<int>(bit / 4)},
                            (mask & (1U << bit)) != 0);
        const auto path = find_path(map, {0, 0}, {3, 3});
        const int expected = breadth_first_distance(map, {0, 0}, {3, 3});
        if (expected < 0) {
            CHECK(path.empty());
            continue;
        }
        CHECK(path.size() == static_cast<std::size_t>(expected + 1));
        CHECK(path.front() == GridCell{0, 0});
        CHECK(path.back() == GridCell{3, 3});
        CHECK(path == find_path(map, {0, 0}, {3, 3}));
        for (std::size_t i = 1; i < path.size(); ++i) {
            CHECK(map.walkable(path[i]));
            CHECK(std::abs(path[i].x - path[i - 1].x) +
                  std::abs(path[i].y - path[i - 1].y) == 1);
        }
    }
}

void test_sight() {
    NavigationGrid grid(5, 5);
    CHECK(has_line_of_sight(grid, {0, 0}, {4, 4}));
    CHECK(has_line_of_sight(grid, {0, 2}, {4, 2}));
    CHECK(has_line_of_sight(grid, {2, 0}, {2, 4}));
    CHECK(has_line_of_sight(grid, {1, 1}, {1, 1}));
    CHECK(!has_line_of_sight(grid, {-1, 0}, {1, 0}));
    grid.set_blocked({1, 0});
    CHECK(!has_line_of_sight(grid, {0, 0}, {1, 1})); // Corner peeking is blocked.
    grid.set_blocked({1, 0}, false);
    grid.set_blocked({2, 2});
    CHECK(!has_line_of_sight(grid, {0, 2}, {4, 2}));
    CHECK(!has_line_of_sight(grid, {2, 0}, {2, 4}));
    CHECK(!has_line_of_sight(grid, {2, 2}, {2, 2}));
    CHECK(has_line_of_sight(grid, {0, 0}, {4, 0}));

    // Visibility must be symmetric in all octants, including shallow/steep rays.
    for (int a = 0; a < 25; ++a) {
        for (int b = 0; b < 25; ++b) {
            CHECK(has_line_of_sight(grid, {a % 5, a / 5}, {b % 5, b / 5}) ==
                  has_line_of_sight(grid, {b % 5, b / 5}, {a % 5, a / 5}));
        }
    }
}

void test_decisions() {
    AiConfig config;
    config.arrival_radius = 0;
    config.search_duration = 1;
    config.stun_duration = 0.5F;
    config.attack_interval = 0.5F;
    AiController ai(config, {{0, 0}, {100, 0}});
    auto result = ai.update({{0, 0}}, 0.1F);
    CHECK(result.state == AiState::patrol);
    CHECK(result.destination->x == 100);
    result = ai.update({{100, 0}}, 0.1F);
    CHECK(result.destination->x == 0); // Route cycles.

    std::array<AiTarget, 3> targets{{{8, {40, 0}, true}, {3, {-40, 0}, true}, {2, {1, 0}, false}}};
    result = ai.update({{0, 0}, targets}, 0.1F);
    CHECK(result.state == AiState::chase);
    CHECK(result.target == 3); // Equal distances use the lower entity ID.
    CHECK(result.destination->x == -40);
    targets[1].position = {10, 0};
    result = ai.update({{0, 0}, targets}, 0); // Paused updates cannot emit attacks.
    CHECK(!result.attack_requested);
    result = ai.update({{0, 0}, targets}, 0.1F);
    CHECK(result.state == AiState::attack && result.attack_requested);
    CHECK(!result.destination);
    CHECK(!ai.update({{0, 0}, targets}, 0.25F).attack_requested);
    CHECK(ai.update({{0, 0}, targets}, 0.25F).attack_requested);

    // Hidden snapshots cannot change the last known destination.
    for (auto& target : targets) {
        target.visible = false;
        target.position = {999, 999};
    }
    result = ai.update({{0, 0}, targets}, 0.25F);
    CHECK(result.state == AiState::search);
    CHECK(result.destination->x == 10);
    CHECK(result.target == invalid_entity && !result.attack_requested);
    result = ai.update({{10, 0}, targets}, 0.25F);
    CHECK(result.state == AiState::search && !result.destination);
    CHECK(ai.update({{10, 0}, targets}, 0.5F).state == AiState::patrol);

    targets[0] = {8, {40, 0}, true};
    CHECK(ai.update({{0, 0}, targets}, 0.1F).state == AiState::chase);
    result = ai.update({{0, 0}, targets, true}, 0.1F);
    CHECK(result.state == AiState::stunned && !result.destination && !result.attack_requested);
    CHECK(ai.update({{0, 0}, targets}, 0.25F).state == AiState::stunned);
    CHECK(ai.update({{0, 0}, targets}, 0.25F).state == AiState::chase);

    // Missing targets after recovery trigger search; a visible target interrupts it.
    CHECK(ai.update({{0, 0}}, 0.1F).state == AiState::search);
    CHECK(ai.update({{0, 0}, targets}, 0.1F).state == AiState::chase);
    ai.reset();
    CHECK(ai.state() == AiState::patrol);
    CHECK(ai.update({{0, 0}}, 0.1F).state == AiState::patrol);

    config.engage_targets = false;
    AiController peaceful(config);
    CHECK(peaceful.update({{0, 0}, targets}, 1).state == AiState::patrol);
    CHECK(!peaceful.update({{0, 0}}, 1).destination);
    AiController duplicate_route(config, {{0, 0}, {0, 0}});
    CHECK(!duplicate_route.update({{0, 0}}, 1).destination);

    // Reject invalid input before touching state or cooldowns.
    CHECK(throws_as<std::invalid_argument>([&] { ai.update({{0, 0}}, -1); }));
    targets[0].entity = invalid_entity;
    CHECK(throws_as<std::invalid_argument>([&] { ai.update({{0, 0}, targets}, 1); }));
    CHECK(ai.state() == AiState::patrol);
    config.attack_interval = 0;
    CHECK(throws_as<std::invalid_argument>([&] { AiController invalid(config); }));
    CHECK(throws_as<std::invalid_argument>([] {
        AiController invalid({}, {{std::numeric_limits<float>::quiet_NaN(), 0}});
    }));
}

void test_movement_integration() {
    // Exercise the public AI -> World component -> physics path around a wall.
    NavigationGrid grid(3, 3);
    grid.set_blocked({1, 0});
    const auto path = find_path(grid, {0, 0}, {2, 0});
    std::vector<Vec2> points;
    for (const auto cell : path) points.push_back({static_cast<float>(cell.x), static_cast<float>(cell.y)});
    World world;
    const auto entity = world.create();
    auto& follower = world.add<WaypointFollower>(entity, WaypointFollower(points, 0));
    auto& body = world.add<Body>(entity);
    auto* transform = world.find(entity);
    int steps = 0;
    while (const auto destination = follower.update(transform->position)) {
        // Each grid step takes one second here; physics applies the intended velocity.
        body.velocity = {destination->x - transform->position.x, destination->y - transform->position.y};
        integrate(*transform, body, 1);
        CHECK(grid.walkable({static_cast<int>(transform->position.x), static_cast<int>(transform->position.y)}));
        CHECK(++steps < 10);
    }
    CHECK(follower.finished());
    CHECK(near(transform->position.x, 2) && near(transform->position.y, 0));
    CHECK(world.destroy(entity));
    CHECK(world.count<WaypointFollower>() == 0);
    WaypointFollower empty({});
    CHECK(empty.finished() && !empty.update({0, 0}));
    CHECK(throws_as<std::invalid_argument>([] { WaypointFollower invalid({}, -1); }));
}

} // namespace

int main() {
    return run_test([] {
        test_original_patrol();
        test_navigation();
        test_sight();
        test_decisions();
        test_movement_integration();
    });
}
