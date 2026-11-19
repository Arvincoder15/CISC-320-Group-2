#include "demo_walkthrough.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace elemental_coop;
using namespace prototype::verification;

namespace {
std::filesystem::path level_asset(const char* name) {
    return std::filesystem::path(UI_PROTOTYPE_ASSET_DIR) / "levels" / name;
}

bool rejects(const engine::LevelDocument& document) {
    try {
        (void)prototype::playable_level_from_document(document);
    } catch (const prototype::PlayableLevelError&) {
        return true;
    }
    return false;
}

void verify_playable_level_assets() {
    const auto easy = prototype::load_playable_level_file(level_asset("level_1_easy.level.json"));
    const auto medium = prototype::load_playable_level_file(level_asset("level_2_medium.level.json"));
    const auto hard = prototype::load_playable_level_file(level_asset("level_3_hard.level.json"));
    check(easy.name == "Level 1 - Easy" && easy.platforms.size() == 14 &&
          easy.hazards.size() == 3 && easy.gates.size() == 1,
          "Native Easy document maps the existing room geometry");
    check(medium.name == "Level 2 - Medium" && medium.gates.size() == 1 &&
          medium.gates[0].require_distinct_operators,
          "Native Medium document maps its distinct-operator gate");
    check(hard.name == "Level 3 - Hard" && hard.gates.size() == 2 &&
          hard.gates[0].require_distinct_operators && hard.gates[1].require_distinct_operators &&
          hard.gates[0].door.x == 460 && hard.gates[1].door.x == 840 &&
          hard.hazards.size() == 5,
          "Native Hard document maps two sequential distinct-operator gates");
    const auto has_hazard = [&](elemental_coop::Hazard kind) {
        return std::any_of(hard.hazards.begin(), hard.hazards.end(),
                           [kind](const auto& hazard) { return hazard.kind == kind; });
    };
    check(has_hazard(Hazard::lava) && has_hazard(Hazard::water) &&
          has_hazard(Hazard::toxic),
          "Hard includes all three elemental hazard types");

    const auto hard_document = engine::load_level_file(level_asset("level_3_hard.level.json"));
    const auto hard_object = [&](const char* id) {
        return std::find_if(hard_document.objects.begin(), hard_document.objects.end(),
                            [id](const auto& object) { return object.id == id; });
    };
    for (const auto* id : {"step-fire-1", "step-fire-2", "step-fire-3",
                           "step-water-1", "step-water-2", "step-water-3"}) {
        const auto step = hard_object(id);
        check(step != hard_document.objects.end() && step->type == "platform" &&
              step->size.x >= 44 && step->size.x <= 50,
              "Hard ascent uses narrow, supported precision platforms");
    }
    const auto fire_exit = hard_object("exit-fire");
    const auto water_exit = hard_object("exit-water");
    check(fire_exit != hard_document.objects.end() &&
          water_exit != hard_document.objects.end() &&
          fire_exit->parent_id != water_exit->parent_id &&
          std::abs((hard.exits[0].area.x + hard.exits[0].area.w / 2.0F) -
                   (hard.exits[1].area.x + hard.exits[1].area.w / 2.0F)) > 350.0F &&
          hard.exits[0].area.y == 96 && hard.exits[1].area.y == 56,
          "Hard matching exits occupy separated elevated platforms");

    const auto medium_document = engine::load_level_file(level_asset("level_2_medium.level.json"));
    const auto find_object = [&](const char* id) {
        return std::find_if(medium_document.objects.begin(), medium_document.objects.end(),
                            [id](const auto& object) { return object.id == id; });
    };
    const auto fire_step = find_object("step-fire-2");
    const auto water_step = find_object("step-water-2");
    const auto is_centered_single_block = [](const auto& object, float original_left) {
        constexpr float width = 160.0F / 3.0F;
        return object->type == "platform" && object->prefab == "solid" &&
            std::abs(object->position.x - (original_left + width)) < 0.001F &&
            object->position.y == 340.0F &&
            std::abs(object->size.x - width) < 0.001F &&
            object->size.y == 20.0F &&
            std::abs(object->position.x + object->size.x / 2.0F -
                     (original_left + 80.0F)) < 0.001F;
    };
    check(fire_step != medium_document.objects.end() &&
          water_step != medium_document.objects.end() &&
          is_centered_single_block(fire_step, 250.0F) &&
          is_centered_single_block(water_step, 870.0F),
          "Medium's upper Fire and Water platforms are one centered block each");

    for (const auto* definition : {&easy, &medium, &hard}) {
        check(definition->bounds.x == 1280 && definition->bounds.y == 720,
              "Playable definition uses the supported world bounds");
        check(definition->spawns[0].y == 580 && definition->spawns[1].y == 580,
              "Playable spawn positions use feet coordinates");
        check(!definition->platforms.empty() &&
              definition->exits[0].id == "exit-fire" &&
              definition->exits[1].id == "exit-water",
              "Playable definitions include matching exits and platforms");
    }

    auto invalid_link = engine::load_level_file(level_asset("level_2_medium.level.json"));
    for (auto& object : invalid_link.objects) {
        if (object.type == "pressure_plate") object.linked_id = "missing-door";
    }
    check(rejects(invalid_link), "Reject dangling game-specific gate links");

    auto unsupported = engine::load_level_file(level_asset("level_1_easy.level.json"));
    unsupported.objects.front().type = "moving_platform";
    check(rejects(unsupported), "Reject unsupported playable object types");

    auto unsupported_link = engine::load_level_file(level_asset("level_2_medium.level.json"));
    for (auto& object : unsupported_link.objects) {
        if (object.type == "hazard") object.linked_id = "door-medium";
    }
    check(rejects(unsupported_link), "Reject unsupported links on non-control objects");

    auto unsafe_spawn = engine::load_level_file(level_asset("level_3_hard.level.json"));
    unsafe_spawn.spawns[0].position.x = 0;
    check(rejects(unsafe_spawn), "Reject a spawn whose player collider is outside the world");

    auto broken_gate = engine::load_level_file(level_asset("level_3_hard.level.json"));
    for (auto& object : broken_gate.objects) {
        if (object.id == "door-gate-b") object.position.x = 600;
    }
    check(rejects(broken_gate), "Reject invalid sequential-gate geometry");
}

void verify_failed_load_preserves_active_level(const prototype::PlayableLevelDefinition& easy) {
    prototype::DemoLevel level(easy);
    solve(level);
    const auto before = level.position(0);
    const auto bad_file = std::filesystem::temp_directory_path() /
        ("invalid-playable-level-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    {
        std::ofstream output(bad_file, std::ios::binary);
        output << "{invalid";
    }
    bool failed = false;
    try {
        level.load_file(bad_file);
    } catch (const std::exception&) {
        failed = true;
    }
    std::filesystem::remove(bad_file);
    check(failed && level.definition().name == easy.name &&
          level.state() == RoundState::won &&
          level.position(0).x == before.x && level.position(0).y == before.y,
          "Failed native reload preserves the active runtime and player state");
}

void verify_hazard_rules(const prototype::PlayableLevelDefinition& easy) {
    for (std::size_t player = 0; player < player_count; ++player) {
        for (const auto hazard : {Hazard::lava, Hazard::water, Hazard::toxic}) {
            prototype::DemoLevel level(easy);
            level.touch_hazard(player, hazard);
            const bool safe = (player == 0 && hazard == Hazard::lava) ||
                              (player == 1 && hazard == Hazard::water);
            check((level.state() == RoundState::playing) == safe,
                  "Elemental hazard immunity and death rules");
        }
    }
}

void verify_hard_retry_after_death(const prototype::PlayableLevelDefinition& hard) {
    prototype::DemoLevel level(hard);
    for (int tick = 0; tick < 240 && level.state() == RoundState::playing; ++tick) {
        PlayerInputs input{};
        input[1].move_x = 1.0F;
        level.tick(input, 1.0F / 120.0F);
    }
    check(level.state() == RoundState::lost,
          "Water physically falls into the opening lava trench and loses");
    level.restart();
    check(level.state() == RoundState::playing && !level.gate_open(0) &&
          !level.gate_open(1) && !level.gate_lever_on(0) &&
          !level.gate_lever_on(1) && level.position(0).x == 70 &&
          level.position(1).x == 110,
          "Retry after Hard hazard death restores safe spawns and resets both gates");
}
}

int main() {
    try {
        verify_playable_level_assets();
        const auto easy = prototype::load_playable_level_file(level_asset("level_1_easy.level.json"));
        const auto medium_definition =
            prototype::load_playable_level_file(level_asset("level_2_medium.level.json"));
        const auto hard_definition =
            prototype::load_playable_level_file(level_asset("level_3_hard.level.json"));

        verify_hazard_rules(easy);
        prototype::DemoLevel easy_level(easy);
        easy_level.interact();
        check(!easy_level.lever(), "Lever cannot activate remotely");
        solve(easy_level);
        easy_level.restart();
        check(!easy_level.open() && !easy_level.lever() && !easy_level.plate(),
              "Easy restart resets puzzle and door state");
        check(easy_level.position(0).x == 150 && easy_level.position(1).x == 1130,
              "Easy restart restores original spawn positions");
        verify_failed_load_preserves_active_level(easy);

        prototype::DemoLevel medium_bypass(medium_definition);
        reach_medium_exits_without_unlocking(medium_bypass);
        prototype::DemoLevel medium(medium_definition);
        solve_medium(medium);
        medium.restart();
        check(!medium.open() && !medium.gate_lever_on(0) &&
              !medium.gate_plate_occupied(0),
              "Medium restart resets its active puzzle only");
        check(medium.position(0).x == 120 && medium.position(1).x == 1160,
              "Medium restart restores its data-driven spawns");

        prototype::DemoLevel hard(hard_definition);
        solve_hard(hard);
        hard.restart();
        check(!hard.gate_open(0) && !hard.gate_open(1) &&
              !hard.gate_lever_on(0) && !hard.gate_lever_on(1),
              "Hard restart resets both gates and their levers");
        check(hard.position(0).x == 70 && hard.position(1).x == 110,
              "Hard restart restores its data-driven spawns");
        verify_hard_retry_after_death(hard_definition);

        std::cout << "PASS: native JSON, semantic validation, safe loading, three physical "
                     "walkthroughs, hazards, cooperative gates, exits, and restart\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
