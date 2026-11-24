#pragma once
#include "elemental_coop/game.hpp"
#include <optional>
#include <stdexcept>

namespace elemental_coop {
// Pure gameplay policy: no rendering, physics, or network dependencies beyond game types.
inline constexpr bool hazard_is_safe(Element element, Hazard hazard) noexcept {
    return (element == Element::fire && hazard == Hazard::lava) ||
           (element == Element::water && hazard == Hazard::water);
}

inline bool cooperative_exit_ready(const std::array<PlayerState, player_count>& players,
                                   bool gates_open = true) noexcept {
    if (!gates_open) return false;
    for (const auto& player : players)
        if (!player.alive || !player.at_exit) return false;
    return true;
}

// Gates latch open until the level is restarted. Geometry stays in the level adapter.
struct GateState {
    bool lever_on{};
    bool open{};
    std::optional<std::size_t> lever_operator;
    std::optional<std::size_t> plate_operator;

    void interact(std::size_t player, bool distinct) {
        if (player >= player_count) throw std::out_of_range("Invalid lever operator");
        // Distinct-player puzzles latch the lever and remember the first operator.
        if (distinct && lever_on) return;
        lever_on = !lever_on;
        lever_operator = lever_on ? std::optional<std::size_t>{player} : std::nullopt;
    }

    // Refresh occupancy each tick. Prefer a different operator when required.
    // Return true only on the closed-to-open transition, so the adapter can remove the door once.
    bool update_plate(const std::array<bool, player_count>& occupants, bool distinct) {
        plate_operator.reset();
        for (std::size_t i = 0; i < occupants.size(); ++i) {
            if (!occupants[i]) continue;
            if (!plate_operator) plate_operator = i;
            if (!distinct || !lever_operator || *lever_operator != i) {
                plate_operator = i;
                break;
            }
        }
        const bool eligible = !distinct || (lever_operator && plate_operator &&
                                              *lever_operator != *plate_operator);
        if (open || !lever_on || !plate_operator || !eligible) return false;
        open = true;
        return true;
    }
};
} // namespace elemental_coop
