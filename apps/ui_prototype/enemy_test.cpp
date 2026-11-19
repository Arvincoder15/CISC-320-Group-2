#include "demo_level.hpp"
#include "test_support.hpp"
#include <filesystem>
#include <string_view>

namespace {
prototype::PlayableLevelDefinition arena() {
    prototype::PlayableLevelDefinition definition;
    definition.name = "NPC integration fixture";
    definition.bounds = {1280, 720};
    // Two flush platforms form one continuous floor from x=300 to x=740.
    definition.platforms = {{300, 580, 180, 70}, {480, 580, 260, 70},
                            {0, 580, 200, 70}, {1000, 580, 280, 70}};
    definition.spawns = {{{100, 580}, {1180, 580}}};
    definition.enemy_spawns = {{"acid_enemy", {512, 580}, "acid-1"}};
    return definition;
}

void verify_hard_pursuit() {
    prototype::PlayableLevelDefinition definition;
    definition.name = "Hunter test";
    definition.hard_enemy_pursuit = true;
    definition.bounds = {1280, 720};
    definition.platforms = {{80, 580, 320, 70}, {465, 540, 270, 70}};
    definition.spawns = {{{540, 540}, {695, 540}}};
    for (int i = 0; i < 5; ++i)
        definition.enemy_spawns.push_back({"acid_enemy", {112.0F + i * 48, 580},
                                          "hunter-" + std::to_string(i)});

    // A closed wall between the platforms prevents the same planned jump.
    const prototype::AcidPursuit open(definition, {});
    auto approach = engine::Vec2{384, 580};
    auto across = open.next(approach, definition.spawns[0]);
    for (int step = 0; step < 4 && across && !across->jump; ++step) {
        approach.x = across->x;
        across = open.next(approach, definition.spawns[0]);
    }
    CHECK(across && across->jump);
    const std::array<prototype::Rect, 1> closed_door{{{415, 250, 30, 330}}};
    const prototype::AcidPursuit closed(definition, closed_door);
    const auto blocked = closed.next({384, 580}, definition.spawns[0]);
    CHECK(blocked && !blocked->jump && blocked->x <= 408);

    for (float dt : {1.0F / 120, 1.0F / 30}) {
        prototype::DemoLevel level(definition);
        bool jumped = false;
        for (int tick = 0; tick < static_cast<int>(8 / dt) &&
             level.state() == elemental_coop::RoundState::playing; ++tick) {
            level.tick({}, dt);
            std::size_t fire_count = 0, water_count = 0;
            for (std::size_t i = 0; i < level.enemies().size(); ++i) {
                const auto& enemy = level.enemies()[i];
                CHECK(!enemy.patrol);
                fire_count += enemy.pursuit_target == 0;
                water_count += enemy.pursuit_target == 1;
                jumped = jumped || enemy.active_jump.has_value();
            }
            CHECK(fire_count >= 2 && water_count >= 2);
            if (tick == 0) {
                // Everyone starts closer to Fire. The smallest penalty for
                // switching targets belongs to the two rightmost hunters.
                CHECK(fire_count == 3 && water_count == 2);
                CHECK(level.enemies()[0].pursuit_target == 0);
                CHECK(level.enemies()[4].pursuit_target == 1);
            }
        }
        CHECK(jumped);
        CHECK(level.state() == elemental_coop::RoundState::lost);
        level.restart();
        CHECK(!level.enemies()[0].active_jump && !level.enemies()[0].pursuit_target);
    }
}

void verify_narrow_platform_pursuit() {
    prototype::PlayableLevelDefinition definition;
    definition.hard_enemy_pursuit = true;
    definition.bounds = {1280, 720};
    // Both floating ledges are narrower than the slime's 32-pixel collider.
    definition.platforms = {{80, 580, 195, 70}, {320, 430, 24, 12}, {430, 282, 28, 12}};
    definition.spawns = {{{444, 282}, {450, 282}}};
    definition.enemy_spawns = {{"acid_enemy", {240, 580}, "hunter"}};
    definition.gates.push_back({"remote", {1100, 450, 25, 100},
        "lever", {330, 410, 12, 20}, "plate", {320, 426, 24, 4}, true});

    const prototype::AcidPursuit pursuit(definition, {});
    const auto across_switches = pursuit.next({316, 430}, {347, 430});
    CHECK(across_switches && !across_switches->jump && near(across_switches->x, 347));

    for (float dt : {1.0F / 120, 1.0F / 30}) {
        prototype::DemoLevel level(definition);
        bool used_narrow_ledge = false;
        for (int tick = 0; tick < static_cast<int>(13 / dt) &&
             level.state() == elemental_coop::RoundState::playing; ++tick) {
            level.tick({}, dt);
            const auto& hunter = level.enemies()[0];
            const auto* contacts = level.world().get<engine::ContactState>(hunter.entity);
            const auto feet = level.enemy_position(0);
            used_narrow_ledge = used_narrow_ledge ||
                (contacts && contacts->grounded && feet.x >= 311.9F && feet.x <= 352.1F && near(feet.y, 430));
            CHECK(!level.gate_plate_occupied(0) && !level.gate_lever_on(0));
        }
        if (!used_narrow_ledge) {
            const auto p = level.enemy_position(0);
            std::cerr << "Narrow ledge dt=" << dt << " position=" << p.x << "," << p.y
                      << " state=" << static_cast<int>(level.state()) << '\n';
        }
        CHECK(used_narrow_ledge);
        CHECK(level.state() == elemental_coop::RoundState::lost);
    }
}

void verify_hard_exit_pursuit() {
    const auto path = std::filesystem::path(UI_PROTOTYPE_ASSET_DIR) / "levels" / "level_3_hard.level.json";
    auto definition = prototype::load_playable_level_file(path);
    definition.gates.clear(); // Both gates have opened before players reach the exits.
    definition.enemy_spawns = {{"acid_enemy", {968, 580}, "hunter"}};
    for (const auto& exit : definition.exits) {
        const engine::Vec2 goal{exit.area.x + exit.area.w / 2, exit.area.y + exit.area.h};
        definition.spawns = {goal, goal};
        prototype::DemoLevel level(definition);
        for (int tick = 0; tick < 3600 && level.state() == elemental_coop::RoundState::playing; ++tick)
            level.tick({}, 1.0F / 120);
        if (level.state() != elemental_coop::RoundState::lost) {
            const auto p = level.enemy_position(0);
            std::cerr << "Hunter stopped at " << p.x << "," << p.y << " pursuing " << exit.id << '\n';
        }
        CHECK(level.state() == elemental_coop::RoundState::lost);
    }
}

}

int main() { return run_test([] {
    using elemental_coop::RoundState;
    verify_hard_pursuit();
    verify_narrow_platform_pursuit();
    verify_hard_exit_pursuit();
    const auto base = arena();
    prototype::DemoLevel patrol(base);
    CHECK(patrol.enemies().size() == 1);
    const auto entity = patrol.enemies()[0].entity;
    CHECK(patrol.world().alive(entity));
    CHECK(patrol.world().get<engine::Body>(entity));
    CHECK(patrol.world().get<engine::Collider>(entity));
    float minimum = 512, maximum = 512;
    for (int tick = 0; tick < 900; ++tick) {
        patrol.tick({}, 1.0F / 120);
        const auto p = patrol.enemy_position(0);
        minimum = std::min(minimum, p.x);
        maximum = std::max(maximum, p.x);
        CHECK(p.x >= 320 && p.x <= 720 && near(p.y, 580));
        CHECK(patrol.enemies()[0].patrol->controller.state() == engine::AiState::patrol);
    }
    CHECK(minimum < 323 && maximum > 717); // Both patrol endpoints were reached.
    CHECK(patrol.enemies()[0].animation_time > 0);
    patrol.restart();
    CHECK(near(patrol.enemy_position(0).x, 512));
    CHECK(patrol.enemies()[0].animation_time == 0);
    CHECK(patrol.enemies()[0].patrol->controller.state() == engine::AiState::patrol);

    // Direct runtime loads also reject a spawn across a flush tile seam.
    auto shared_floor = base;
    shared_floor.spawns[0] = {325, 580};
    bool rejected_shared_floor = false;
    try {
        patrol.load(shared_floor);
    } catch (const prototype::PlayableLevelError& error) {
        rejected_shared_floor = std::string_view(error.what()).find("shares a starting floor") != std::string_view::npos;
    }
    CHECK(rejected_shared_floor);
    CHECK(near(patrol.position(0).x, base.spawns[0].x));

    // Both players start on safe ledges and are chased only after dropping down.
    for (std::size_t who = 0; who < 2; ++who) {
        auto definition = base;
        definition.platforms.push_back({who == 0 ? 740.0F : 256.0F, 530, 44, 12});
        definition.spawns[who] = {who == 0 ? 754.0F : 286.0F, 530};
        prototype::DemoLevel chase(definition);
        chase.tick({}, 1.0F / 120);
        CHECK(chase.enemies()[0].patrol->controller.state() == engine::AiState::patrol);
        elemental_coop::PlayerInputs inputs{};
        inputs[who].move_x = who == 0 ? -1 : 1;
        for (int tick = 0; tick < 18; ++tick) chase.tick(inputs, 1.0F / 120);
        for (int tick = 0; tick < 60 &&
             chase.enemies()[0].patrol->controller.state() != engine::AiState::chase; ++tick)
            chase.tick({}, 1.0F / 120);
        CHECK(chase.enemies()[0].patrol->controller.state() == engine::AiState::chase);
        for (int tick = 0; tick < 480 && chase.state() == RoundState::playing; ++tick)
            chase.tick({}, 1.0F / 120);
        CHECK(chase.state() == RoundState::lost);
        CHECK(!chase.player(who).alive);
        const auto stopped = chase.enemy_position(0);
        chase.tick({}, 1);
        CHECK(near(chase.enemy_position(0).x, stopped.x));
        chase.restart();
        CHECK(chase.state() == RoundState::playing && chase.player(who).alive);
        CHECK(near(chase.enemy_position(0).x, 512));
    }

    // Real input can jump across an enemy's territory without touching its body.
    auto jump_definition = base;
    jump_definition.platforms.push_back({400, 530, 44, 12});
    jump_definition.spawns[0] = {432, 530};
    prototype::DemoLevel jumping(jump_definition);
    for (int tick = 0; tick < 110; ++tick) {
        elemental_coop::PlayerInputs inputs{};
        inputs[0].move_x = 1;
        inputs[0].primary_pressed = tick == 0;
        jumping.tick(inputs, 1.0F / 120);
        CHECK(jumping.state() == RoundState::playing);
    }
    CHECK(jumping.position(0).x > 630);

    // The adapter uses body clearance, live gates, support and hazards as obstacles.
    const engine::Vec2 home{512, 580};
    prototype::AcidNavigation open(base, home, {});
    CHECK(open.shares_floor({480,580}, {739,580}));
    CHECK(open.shares_floor({544,580}, {290,580})); // Player can overhang the edge.
    CHECK(!open.shares_floor({480,580}, {544,460}));
    CHECK(!open.shares_floor({480,580}, {1180,580})); // Different floor at the same height.
    CHECK(open.next({480,580}, {544,580}).has_value());
    const std::array<prototype::Rect, 1> door{{{508,500,8,80}}};
    prototype::AcidNavigation closed(base, home, door);
    CHECK(!closed.shares_floor({480,580}, {544,580}));
    CHECK(!closed.next({480,580}, {544,580}).has_value());
    auto gap = base;
    gap.platforms = {{0,580,500,70}, {524,580,756,70}};
    prototype::AcidNavigation disconnected(gap, home, {});
    CHECK(!disconnected.next({480,580}, {544,580}).has_value());
    auto hazard = base;
    hazard.hazards = {{{508,548,8,32}, elemental_coop::Hazard::toxic}};
    prototype::AcidNavigation dangerous(hazard, home, {});
    CHECK(!dangerous.next({480,580}, {544,580}).has_value());

    std::size_t expected = 1;
    for (const auto* file : {"level_1_easy.level.json", "level_2_medium.level.json", "level_3_hard.level.json"}) {
        const auto path = std::filesystem::path(UI_PROTOTYPE_ASSET_DIR) / "levels" / file;
        auto document = engine::load_level_file(path);
        const auto definition = prototype::playable_level_from_document(document);
        prototype::DemoLevel level(definition);
        CHECK(level.enemies().size() == expected);
        for (std::size_t i = 0; i < level.enemies().size(); ++i) {
            CHECK(near(level.enemy_position(i).x, definition.enemy_spawns[i].position.x));
            CHECK(near(level.enemy_position(i).y, definition.enemy_spawns[i].position.y));
        }
        // The former Medium spawns entered pursuit immediately and could kill idle players.
        for (int tick = 0; tick < (definition.hard_enemy_pursuit ? 60 : 1200); ++tick) {
            level.tick({}, 1.0F / 120);
            CHECK(level.state() == RoundState::playing);
        }
        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            CHECK(near(level.position(i).x, definition.spawns[i].x));
            CHECK(near(level.position(i).y, definition.spawns[i].y));
        }
        expected += 2;
        const auto rejected = [](const engine::LevelDocument& invalid) {
            try { (void)prototype::playable_level_from_document(invalid); }
            catch (const prototype::PlayableLevelError&) { return true; }
            return false;
        };
        const auto enemy_index = document.spawns.size() - 1;
        auto invalid = document;
        invalid.spawns[enemy_index].position = definition.spawns[0];
        CHECK(rejected(invalid));
        invalid = document;
        invalid.spawns[enemy_index].position.y -= 10; // Floating enemy.
        CHECK(rejected(invalid));
        invalid = document;
        invalid.spawns[enemy_index].position.x = -50;
        CHECK(rejected(invalid));
        invalid = document;
        if (definition.name == "Level 1 - Easy") {
            // Each position is more than 80 pixels from the player, but shares their floor.
            for (float x : {240.0F, 1248.0F}) {
                invalid.spawns[enemy_index].position = {x, 580};
                CHECK(rejected(invalid));
            }
        }
        invalid = document;
        invalid.spawns.push_back(invalid.spawns[enemy_index]);
        CHECK(rejected(invalid));
    }
}); }
