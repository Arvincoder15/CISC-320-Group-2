#include "engine/ai.hpp"
#include <array>
#include <iostream>

namespace {
const char* state_name(engine::AiState state) {
    switch (state) {
    case engine::AiState::patrol: return "patrol";
    case engine::AiState::chase: return "chase";
    case engine::AiState::attack: return "attack";
    case engine::AiState::search: return "search";
    case engine::AiState::stunned: return "stunned";
    }
    return "unknown";
}

void print_decision(const char* event, const engine::AiDecision& decision) {
    std::cout << event << " -> " << state_name(decision.state);
    if (decision.destination)
        std::cout << " destination=(" << decision.destination->x << ',' << decision.destination->y << ')';
    if (decision.target) std::cout << " target=" << decision.target;
    if (decision.attack_requested) std::cout << " ATTACK requested";
    std::cout << '\n';
}
} // namespace

int main() {
    engine::NavigationGrid grid(7, 5);
    for (int y = 0; y < 4; ++y) grid.set_blocked({3, y});
    const engine::GridCell start{1, 1};
    const engine::GridCell goal{5, 1};
    const auto path = engine::find_path(grid, start, goal);
    if (path.empty()) return 1;
    std::cout << "A* route around a wall (# wall, * path, S start, G goal)\n";
    for (int y = 0; y < grid.height(); ++y) {
        for (int x = 0; x < grid.width(); ++x) {
            const engine::GridCell cell{x, y};
            char tile = grid.walkable(cell) ? '.' : '#';
            for (const auto step : path) if (step == cell) tile = '*';
            if (cell == start) tile = 'S';
            if (cell == goal) tile = 'G';
            std::cout << tile;
        }
        std::cout << '\n';
    }
    std::cout << "Path length: " << path.size() - 1 << " steps\n";
    std::cout << "Direct line of sight: " << (engine::has_line_of_sight(grid, start, goal) ? "clear" : "blocked") << "\n\n";

    // ECS positions and visibility queries supply these values
    engine::AiController enemy({}, {{0, 0}, {100, 0}});
    std::array<engine::AiTarget, 1> targets{{{2, {80, 0}, true}}};
    print_decision("No target", enemy.update({{0, 0}}, 0.1F));
    print_decision("Target detected", enemy.update({{0, 0}, targets}, 0.1F));
    targets[0].position = {10, 0};
    print_decision("Target in range", enemy.update({{0, 0}, targets}, 0.1F));
    print_decision("Attack on cooldown", enemy.update({{0, 0}, targets}, 0.1F));
    print_decision("Damage received", enemy.update({{0, 0}, targets, true}, 0.1F));
    print_decision("Recovered", enemy.update({{0, 0}, targets}, 0.5F));
    targets[0].visible = false;
    print_decision("Target lost", enemy.update({{0, 0}, targets}, 0.1F));
    print_decision("Search expired", enemy.update({{0, 0}, targets}, 3.0F));
}
