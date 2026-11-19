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

inline void travel(prototype::DemoLevel& level, std::size_t who, float target_x, bool jump) {
    const auto start = level.position(who);
    bool jump_sent = !jump;
    for (int tick = 0; tick < 480; ++tick) {
        const auto position = level.position(who);
        PlayerInputs input{};
        const float distance = target_x - position.x;
        if (std::abs(distance) > 2.5F) input[who].move_x = distance > 0 ? 1.0F : -1.0F;
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
    travel(level, 1, 800, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1125, false);
    check(level.state() == RoundState::won, "Both Easy matching exits win");
}

inline void reach_medium_exits_without_unlocking(prototype::DemoLevel& level) {
    travel(level, 0, 220, false);
    travel(level, 0, 430, true);
    travel(level, 0, 520, false);
    travel(level, 0, 400, false);
    travel(level, 0, 520, true);
    travel(level, 0, 330, true);
    travel(level, 0, 125, true);
    travel(level, 1, 1030, false);
    travel(level, 1, 770, true);
    travel(level, 1, 835, false);
    travel(level, 1, 720, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1160, true);
    check(std::abs(level.position(0).x - 125.0F) <= 3.0F &&
          std::abs(level.position(1).x - 1160.0F) <= 3.0F,
          "Both players physically reach the locked Medium exits");
    check(level.state() == RoundState::playing && !level.open(),
          "Reaching both exits cannot bypass the Medium gate");
}

inline void solve_medium(prototype::DemoLevel& level) {
    travel(level, 0, 220, false);
    travel(level, 0, 430, true);
    travel(level, 0, 520, false);
    press_interact(level);
    check(level.gate_lever_on(0) && !level.gate_open(0),
          "Medium gate remains closed after its lever alone");

    travel(level, 0, 400, false);
    travel(level, 0, 520, true);
    travel(level, 0, 720, true);
    travel(level, 0, 790, false);
    check(level.gate_plate_occupied(0) && !level.gate_open(0),
          "The same Medium player cannot operate both controls");
    travel(level, 0, 820, false);

    travel(level, 1, 1030, false);
    travel(level, 1, 770, true);
    check(level.gate_open(0), "Distinct Medium operators open the gate");
    travel(level, 0, 835, false);
    travel(level, 0, 720, true);
    travel(level, 0, 520, true);
    travel(level, 0, 330, true);
    travel(level, 0, 125, true);
    check(level.state() == RoundState::playing, "One Medium matching exit is insufficient");
    travel(level, 1, 835, false);
    travel(level, 1, 720, true);
    travel(level, 1, 950, true);
    travel(level, 1, 1160, true);
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

    travel(level, 0, 206, true);
    check(level.player(0).grounded &&
          std::abs(level.position(0).y - 470.0F) < 1.0F,
          "Fire reaches the narrow elevated Gate A lever platform");
    press_interact(level);
    check(level.gate_lever_on(0) && !level.gate_open(0),
          "Hard Gate A remains closed after its lever alone");
    push_against_gate(level, 0, level.definition().gates[0].door);
    check(!level.gate_open(0),
          "Gate A cannot be bypassed by jumping from its elevated lever ledge");
    travel(level, 0, 380, false);
    check(level.gate_plate_occupied(0) && !level.gate_open(0),
          "The same Hard Gate A operator cannot activate both controls");
    travel(level, 0, 430, false);
    push_against_gate(level, 0, level.definition().gates[0].door);
    check(!level.gate_open(0) && !level.gate_lever_on(1),
          "Gate A blocks physical access to Gate B's controls");

    travel(level, 1, 378, true);
    check(level.gate_open(0) && !level.gate_open(1),
          "Water crosses the lava gap and opens Gate A with Fire");
    travel(level, 0, 450, false);
    travel(level, 0, 560, true);
    check(level.gate_plate_occupied(1) && !level.gate_open(1),
          "Gate B's raised plate alone leaves its door closed");
    travel(level, 0, 610, false);

    travel(level, 1, 610, true);
    travel(level, 1, 695, true);
    travel(level, 1, 809, true);
    press_interact(level);
    check(level.gate_lever_on(1) && !level.gate_open(1),
          "Water latches Gate B's separate elevated lever");
    push_against_gate(level, 1, level.definition().gates[1].door);
    check(!level.gate_open(1) && level.state() == RoundState::playing,
          "Closed Gate B blocks the final route");

    travel(level, 1, 695, false);
    travel(level, 1, 560, true);
    check(level.gate_plate_occupied(1) && !level.gate_open(1),
          "The Gate B lever operator cannot open it alone from the plate");
    travel(level, 1, 610, false);
    travel(level, 0, 695, true);
    travel(level, 0, 560, true);
    check(level.gate_open(0) && level.gate_open(1),
          "Fire and Water open Gate B with distinct operators after Gate A");
    travel(level, 1, 695, true);
    travel(level, 1, 809, true);
    travel(level, 1, 930, true);
    check(level.gate_open(0) && level.gate_open(1),
          "Both Hard gates remain latched open after leaving their controls");

    travel(level, 0, 610, false);
    travel(level, 0, 695, true);
    travel(level, 0, 809, true);
    travel(level, 0, 930, true);
    travel(level, 0, 870, false);
    travel(level, 0, 975, true);
    travel(level, 0, 1010, false);
    travel(level, 0, 913, true);
    travel(level, 0, 890, false);
    travel(level, 0, 1003, true);
    travel(level, 0, 980, false);
    travel(level, 0, 845, true);
    check(level.player(0).at_exit && level.state() == RoundState::playing,
          "Fire completes its narrow, multi-step ascent but waits for Water");

    travel(level, 1, 1030, false);
    travel(level, 1, 1144, true);
    travel(level, 1, 1120, false);
    travel(level, 1, 1242, true);
    travel(level, 1, 1264, false);
    travel(level, 1, 1103, true);
    travel(level, 1, 1080, false);
    travel(level, 1, 1255, true);
    check(level.state() == RoundState::won,
          "Water crosses its hazard route and reaches its separated exit to finish Hard");
}

} // namespace prototype::verification
