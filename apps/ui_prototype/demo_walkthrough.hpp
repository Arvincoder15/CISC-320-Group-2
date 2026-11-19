#pragma once

#include "demo_level.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

namespace prototype::verification {

using namespace elemental_coop;

inline void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

inline void travel(prototype::DemoLevel& level, std::size_t who, float target_x, bool jump, int vertical_ticks = 0) {
    const auto start = level.position(who);
    bool jump_sent = !jump;
    for (int tick = 0; tick < 480; ++tick) {
        const auto position = level.position(who);
        PlayerInputs input{};
        const float distance = target_x - position.x;
        if (tick >= vertical_ticks && std::abs(distance) > 2.5F) input[who].move_x = distance > 0 ? 1.0F : -1.0F;
        if (jump && !jump_sent && level.player(who).grounded) {
            input[who].primary_pressed = true;
            jump_sent = true;
        }
        level.tick(input, 1.0F / 120.0F);
        if (level.state() == RoundState::won) return;
        if (level.state() == RoundState::lost) {
            throw std::runtime_error("Physical route hit a hazard for player " +
                                     std::to_string(who) + " in " +
                                     level.definition().name + " from " +
                                     std::to_string(start.x) + "," +
                                     std::to_string(start.y) + " at " +
                                     std::to_string(level.position(who).x) + "," +
                                     std::to_string(level.position(who).y) + " heading to " +
                                     std::to_string(target_x));
        }
        const auto landed = level.position(who);
        if (jump_sent && level.player(who).grounded &&
            std::abs(landed.x - target_x) <= 3.0F) return;
    }
    const auto stopped = level.position(who);
    throw std::runtime_error("Physical route stuck for player " + std::to_string(who) +
                             " in " + level.definition().name + " from " +
                             std::to_string(start.x) + "," + std::to_string(start.y) +
                             " at " + std::to_string(stopped.x) + "," +
                             std::to_string(stopped.y) + " heading to " +
                             std::to_string(target_x));
}

inline void press_interact(prototype::DemoLevel& level) {
    level.interact();
    level.tick({}, 1.0F / 120.0F);
}

inline void solve(prototype::DemoLevel& level) {
    travel(level, 0, 230, false);
    travel(level, 0, 485, true);
    press_interact(level);
    check(level.lever(), "Nearby player activates the Easy lever");
    check(!level.open(), "Easy lever alone cannot open the door");
    travel(level, 1, 1060, false);
    travel(level, 1, 805, true);
    check(level.plate() && level.open(), "Easy lever and plate unlock the door");
    travel(level, 0, 340, true);
    travel(level, 0, 480, true);
    travel(level, 0, 320, true);
    travel(level, 0, 155, false);
    check(level.player(0).at_exit && level.state() == RoundState::playing,
          "One Easy exit is insufficient");
    travel(level, 1, 940, true);
    travel(level, 1, 870, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1125, false);
    check(level.state() == RoundState::won, "Both Easy matching exits win");
}

// Both players drop from their safe ledges, then jump past the lower patrols.
inline void leave_medium_spawns(prototype::DemoLevel& level) {
    const std::array<float, 2> launch{220, 1030};
    const std::array<float, 2> landing{430, 835};
    std::array<bool, 2> jumped{};
    for (int tick = 0; tick < 300; ++tick) {
        PlayerInputs input{};
        bool arrived = true;
        for (std::size_t i = 0; i < player_count; ++i) {
            const float x = level.position(i).x;
            if (!jumped[i] && std::abs(x - launch[i]) <= 2 && level.player(i).grounded) {
                input[i].primary_pressed = true;
                jumped[i] = true;
            }
            const float distance = (jumped[i] ? landing[i] : launch[i]) - x;
            if (std::abs(distance) > 2) input[i].move_x = distance > 0 ? 1.0F : -1.0F;
            arrived = arrived && jumped[i] && std::abs(distance) <= 2 && level.player(i).grounded;
        }
        if (arrived) return;
        level.tick(input, 1.0F / 120.0F);
        check(level.state() == RoundState::playing, "Both players escape Medium's starting slimes");
    }
    throw std::runtime_error("Players did not reach Medium's center platforms");
}

inline void reach_medium_exits_without_unlocking(prototype::DemoLevel& level) {
    leave_medium_spawns(level);
    travel(level, 0, 520, false);
    travel(level, 0, 400, false);
    travel(level, 0, 490, true);
    travel(level, 0, 330, true);
    travel(level, 0, 125, true);
    travel(level, 1, 720, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1140, true);
    check(std::abs(level.position(0).x - 125.0F) <= 3.0F &&
          std::abs(level.position(1).x - 1140.0F) <= 3.0F,
          "Both players physically reach the locked Medium exits");
    check(level.state() == RoundState::playing && !level.open(),
          "Reaching both exits cannot bypass the Medium gate");
}

inline void solve_medium(prototype::DemoLevel& level) {
    leave_medium_spawns(level);
    travel(level, 0, 520, false);
    press_interact(level);
    check(level.gate_lever_on(0) && !level.gate_open(0),
          "Medium gate remains closed after its lever alone");

    travel(level, 0, 400, false);
    travel(level, 0, 490, true);
    travel(level, 0, 720, true, 8);
    travel(level, 0, 790, false);
    check(level.gate_plate_occupied(0) && !level.gate_open(0),
          "The same Medium player cannot operate both controls");
    travel(level, 0, 820, false);

    travel(level, 1, 770, false);
    check(level.gate_open(0), "Distinct Medium operators open the gate");
    travel(level, 0, 835, false);
    travel(level, 0, 720, true);
    travel(level, 0, 490, true);
    travel(level, 0, 330, true);
    travel(level, 0, 125, true);
    check(level.state() == RoundState::playing, "One Medium matching exit is insufficient");
    travel(level, 1, 835, false);
    travel(level, 1, 720, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1140, true);
    check(level.state() == RoundState::won, "Both Medium matching exits win");
}

inline void push_against_gate(prototype::DemoLevel& level, std::size_t who, Rect door) {
    float furthest = level.position(who).x;
    for (int tick = 0; tick < 240; ++tick) {
        PlayerInputs input{};
        input[who].move_x = 1.0F;
        input[who].primary_pressed = tick == 0;
        level.tick(input, 1.0F / 120.0F);
        furthest = std::max(furthest, level.position(who).x);
        if (level.state() != RoundState::playing)
            throw std::runtime_error("Closed-gate reachability probe changed round state");
    }
    check(furthest <= door.x - 14.0F + 0.1F,
          "Closed gate blocks the player's full collider");
}

inline void solve_hard(prototype::DemoLevel& level) {
    check(level.gate_count() == 2 && level.gate_open(0) == false &&
          level.gate_open(1) == false,
          "Hard starts with two sequential gates closed");

    // Both players move at once so Fire can leave the lever ledge before the
    // hunter arrives, and Water can clear the plate as soon as the gate opens.
    std::array<int, player_count> leg{};
    const std::array<std::array<float, 2>, player_count> landings{{{206, 500}, {416, 560}}};
    std::array<bool, player_count> jumping{};
    for (int tick = 0; tick < 500 && (leg[0] < 2 || leg[1] < 2); ++tick) {
        PlayerInputs inputs{};
        for (std::size_t who = 0; who < player_count; ++who) {
            if (leg[who] == 2) continue;
            const float dx = landings[who][leg[who]] - level.position(who).x;
            if (jumping[who] && level.player(who).grounded && std::abs(dx) <= 2.5F) {
                jumping[who] = false;
                if (who == 0 && leg[who] == 0) level.interact();
                ++leg[who];
                continue;
            }
            if (std::abs(dx) > 2.5F) inputs[who].move_x = dx > 0 ? 1 : -1;
            if (!jumping[who] && level.player(who).grounded && (who == 0 || tick >= 12)) {
                jumping[who] = true;
                inputs[who].primary_pressed = true;
            }
        }
        level.tick(inputs, 1.0F / 120.0F);
        if (level.state() == RoundState::lost)
            throw std::runtime_error("Hard opening chase reached a player at " +
                std::to_string(level.position(0).x) + "," + std::to_string(level.position(0).y) + " / " +
                std::to_string(level.position(1).x) + "," + std::to_string(level.position(1).y));
    }
    check(leg[0] == 2 && leg[1] == 2 && level.gate_open(0) && !level.gate_open(1),
          "Both Hard players clear Gate A during the opening chase");
    travel(level, 0, 544, true, 8);

    travel(level, 1, 809, true);
    press_interact(level);
    check(level.gate_lever_on(1) && level.gate_open(1),
          "Fire and Water open Gate B with distinct operators after Gate A");

    struct Move { float x; bool jump; };
    const std::array<std::vector<Move>, player_count> escape{{
        {{809, true}, {913, true}, {890, false},
         {1003, true}, {980, false}, {845, true}},
        {{944, true}, {1144, true}, {1120, false}, {1242, true}, {1264, false},
         {1103, true}, {1080, false}, {1255, true}}
    }};
    std::array<std::size_t, player_count> next{};
    jumping = {};
    for (int tick = 0; tick < 1800 && level.state() == RoundState::playing; ++tick) {
        PlayerInputs inputs{};
        for (std::size_t who = 0; who < player_count; ++who) {
            if (next[who] == escape[who].size()) continue;
            const auto move = escape[who][next[who]];
            const float dx = move.x - level.position(who).x;
            if (level.player(who).grounded && std::abs(dx) <= 2.5F && (!move.jump || jumping[who])) {
                ++next[who];
                jumping[who] = false;
                continue;
            }
            if (std::abs(dx) > 2.5F) inputs[who].move_x = dx > 0 ? 1 : -1;
            if (move.jump && !jumping[who] && level.player(who).grounded) {
                jumping[who] = true;
                inputs[who].primary_pressed = true;
            }
        }
        level.tick(inputs, 1.0F / 120.0F);
        if (level.state() == RoundState::lost) {
            std::string diagnostic;
            for (std::size_t who = 0; who < player_count; ++who) {
                const auto p = level.position(who);
                diagnostic += " player " + std::to_string(who) + " at " +
                              std::to_string(p.x) + "," + std::to_string(p.y) +
                              " leg " + std::to_string(next[who]);
            }
            for (std::size_t i = 0; i < level.enemies().size(); ++i) {
                const auto p = level.enemy_position(i);
                diagnostic += " enemy " + std::to_string(p.x) + "," + std::to_string(p.y);
            }
            throw std::runtime_error("Hard exit chase:" + diagnostic);
        }
    }
    check(level.gate_open(0) && level.gate_open(1), "Both Hard gates remain latched open");
    check(level.state() == RoundState::won, "Both players escape the hunters via the Hard exits");
}

} // namespace prototype::verification
