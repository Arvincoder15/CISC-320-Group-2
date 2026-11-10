#include "engine/physics.hpp"
#include "engine/architecture.hpp"
#include "test_support.hpp"
#include <limits>

using namespace engine;

namespace {

void test_integration_and_overlap() {
    Transform transform;
    Body body{{0, 0}, {0, 10}};
    integrate(transform, body, 0.5F);
    CHECK(near(body.velocity.y, 5));
    CHECK(near(transform.position.y, 2.5F));
    integrate(transform, body, 0);
    CHECK(near(transform.position.y, 2.5F));
    CHECK(throws_as<std::invalid_argument>([&] { integrate(transform, body, -1); }));
    CHECK(overlaps({{0, 0}, {10, 10}}, {{9, 9}, {3, 3}}));
    CHECK(!overlaps({{0, 0}, {10, 10}}, {{10, 0}, {3, 3}}));
    CHECK(!overlaps({{0, 0}, {0, 10}}, {{0, 0}, {3, 3}}));
}

void test_collider_bounds_and_layers() {
    // Feet-anchored character: the box sits above and centred on the position.
    Transform transform{{100, 50}};
    const Collider feet{{-8, -32}, {16, 32}};
    const auto box = bounds(transform, feet);
    CHECK(near(box.position.x, 92) && near(box.position.y, 18));
    CHECK(near(box.size.x, 16) && near(box.size.y, 32));

    constexpr CollisionLayers player = 1U << 1U;
    constexpr CollisionLayers ghost = 1U << 2U;
    const Collider a{{}, {1, 1}, player, layer_all};
    const Collider b{{}, {1, 1}, layer_default, layer_default | player};
    const Collider c{{}, {1, 1}, ghost, ghost};
    CHECK(layers_interact(a, b) && layers_interact(b, a));
    CHECK(!layers_interact(a, c) && !layers_interact(c, a));
    CHECK(!layers_interact(Collider{{}, {1, 1}, layer_none}, b));
}

void test_settings_validation() {
    validate(PhysicsSettings{}); // Defaults are valid.
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const auto rejects = [](PhysicsSettings settings) {
        return throws_as<std::invalid_argument>([&] { validate(settings); });
    };
    PhysicsSettings settings;
    settings.gravity = {0, nan};
    CHECK(rejects(settings));
    settings = {};
    settings.gravity = {inf, 0};
    CHECK(rejects(settings));
    settings = {};
    settings.max_speed = {0, 100};
    CHECK(rejects(settings));
    settings = {};
    settings.max_speed = {100, nan};
    CHECK(rejects(settings));
    settings = {};
    settings.max_substep = 0;
    CHECK(rejects(settings));
    settings = {};
    settings.max_substep = inf;
    CHECK(rejects(settings));
    settings = {};
    settings.max_substeps = 0;
    CHECK(rejects(settings));
    settings = {};
    settings.contact_skin = 0;
    CHECK(rejects(settings));
    settings = {};
    settings.max_speed = {200, 600};
    settings.gravity = {0, 900};
    validate(settings);
}

void test_registration_and_removal() {
    World world;
    PhysicsSystem physics;
    const auto a = world.create();
    const auto b = world.create();
    physics.add(world, a);
    physics.add(world, a); // Idempotent.
    physics.add(world, b);
    CHECK(physics.size() == 2 && physics.contains(a) && physics.contains(b));
    CHECK(physics.remove(b) && !physics.remove(b) && !physics.contains(b));
    CHECK(throws_as<std::invalid_argument>([&] { physics.add(world, invalid_entity); }));
    CHECK(throws_as<std::invalid_argument>([&] { physics.add(world, 999); }));
    CHECK(throws_as<std::invalid_argument>([] { PhysicsSystem bad(PhysicsSettings{{}, {}, 0}); }));

    // Destroyed entities are pruned on the next step without touching the World.
    world.add<Body>(a, Body{{10, 0}, {}});
    physics.add(world, b);
    CHECK(world.destroy(a));
    physics.step(world, 1.0F / 60.0F);
    CHECK(physics.size() == 1 && !physics.contains(a) && physics.contains(b));
    CHECK(world.count<Body>() == 0);
}

void test_step_integration() {
    World world;
    PhysicsSettings settings;
    settings.gravity = {0, 600};
    PhysicsSystem physics(settings);
    const auto mover = world.create({{10, 20}});
    const auto fixed = world.create({{5, 5}}); // No Body: never moves.
    world.add<Body>(mover, Body{{60, 0}, {0, 300}});
    physics.add(world, mover);
    physics.add(world, fixed);

    // Matches semi-implicit Euler with body acceleration plus global gravity.
    physics.step(world, 0.5F);
    CHECK(near(world.get<Body>(mover)->velocity.y, 450));
    CHECK(near(world.find(mover)->position.x, 40));
    CHECK(near(world.find(mover)->position.y, 245));
    CHECK(near(world.find(fixed)->position.x, 5) && near(world.find(fixed)->position.y, 5));

    physics.step(world, 0);
    CHECK(near(world.find(mover)->position.y, 245));
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, -1); }));
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, std::numeric_limits<float>::quiet_NaN()); }));
    CHECK(near(world.find(mover)->position.y, 245));

    // Speed limits clamp both axes after acceleration, including an initial overspeed.
    World capped_world;
    settings.max_speed = {100, 500};
    PhysicsSystem capped(settings);
    const auto falling = capped_world.create();
    capped_world.add<Body>(falling, Body{{-250, 0}, {}});
    capped.add(capped_world, falling);
    for (int i = 0; i < 120; ++i) capped.step(capped_world, 1.0F / 60.0F);
    CHECK(near(capped_world.get<Body>(falling)->velocity.x, -100));
    CHECK(near(capped_world.get<Body>(falling)->velocity.y, 500));
}

void test_invalid_components_rejected_before_mutation() {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    World world;
    PhysicsSystem physics;
    const auto good = world.create();
    const auto bad = world.create();
    world.add<Body>(good, Body{{30, 0}, {}});
    auto& collider = world.add<Collider>(bad, Collider{{}, {0, 10}});
    physics.add(world, good);
    physics.add(world, bad);
    const auto unchanged = [&] { return near(world.find(good)->position.x, 0); };

    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, 1); }));
    CHECK(unchanged());
    collider.size = {10, nan};
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, 1); }));
    collider.size = {10, 10};
    collider.offset = {nan, 0};
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, 1); }));
    collider.offset = {};
    world.add<Body>(bad, Body{{0, 0}, {nan, 0}});
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, 1); }));
    world.get<Body>(bad)->acceleration = {};
    world.find(bad)->position.y = nan;
    CHECK(throws_as<std::invalid_argument>([&] { physics.step(world, 1); }));
    CHECK(unchanged());

    world.find(bad)->position.y = 0;
    physics.step(world, 1);
    CHECK(near(world.find(good)->position.x, 30));
}

// Shared helpers for collision scenes.
EntityId add_solid(World& world, PhysicsSystem& physics, Aabb box, CollisionLayers layer = layer_default) {
    const auto id = world.create({box.position});
    world.add<Collider>(id, Collider{{}, box.size, layer});
    physics.add(world, id);
    return id;
}

// A 16x32 character whose Transform position is its feet (bottom centre).
EntityId add_character(World& world, PhysicsSystem& physics, Vec2 feet, Body body = {}) {
    const auto id = world.create({feet});
    world.add<Body>(id, body);
    world.add<Collider>(id, Collider{{-8, -32}, {16, 32}});
    physics.add(world, id);
    return id;
}

void test_walls() {
    World world;
    PhysicsSystem physics;
    add_solid(world, physics, {{100, 0}, {20, 100}}); // Right wall, left face at x = 100.
    add_solid(world, physics, {{-20, 0}, {20, 100}}); // Left wall, right face at x = 0.
    const auto hero = add_character(world, physics, {80, 50}, Body{{600, 0}, {}});

    physics.step(world, 1.0F / 60.0F); // Moves 10 px: right edge reaches 98, no contact.
    CHECK(near(world.find(hero)->position.x, 90));
    CHECK(!world.get<ContactState>(hero)->wall_right);
    physics.step(world, 1.0F / 60.0F); // Would reach 108; stopped flush at 100.
    CHECK(near(world.find(hero)->position.x, 92));
    CHECK(world.get<ContactState>(hero)->wall_right && !world.get<ContactState>(hero)->wall_left);
    CHECK(world.get<Body>(hero)->velocity.x == 0);

    world.get<Body>(hero)->velocity.x = -6000; // 100 px per tick to the left.
    physics.step(world, 1.0F / 60.0F);
    CHECK(near(world.find(hero)->position.x, 8)); // Left edge flush with x = 0.
    CHECK(world.get<ContactState>(hero)->wall_left && !world.get<ContactState>(hero)->wall_right);

    // Flags describe only the latest step.
    physics.step(world, 1.0F / 60.0F);
    CHECK(!world.get<ContactState>(hero)->wall_left);
    CHECK(near(world.find(hero)->position.x, 8));
}

void test_floors_and_ceilings() {
    World world;
    PhysicsSettings settings;
    settings.gravity = {0, 900};
    PhysicsSystem physics(settings);
    const auto floor = add_solid(world, physics, {{0, 320}, {400, 40}});
    const auto hero = add_character(world, physics, {100, 300});
    const auto contacts = [&] { return *world.get<ContactState>(hero); };

    // Landing: falls, stops with feet exactly on the floor top, velocity cleared.
    CHECK(world.get<ContactState>(hero) == nullptr); // Created by the first step.
    int ticks = 0;
    do physics.step(world, 1.0F / 60.0F);
    while (!contacts().grounded && ++ticks < 120);
    CHECK(ticks < 120);
    CHECK(near(world.find(hero)->position.y, 320));
    CHECK(world.get<Body>(hero)->velocity.y == 0);
    CHECK(!contacts().ceiling);

    // Gravity keeps a resting body grounded every tick without sinking.
    for (int i = 0; i < 30; ++i) {
        physics.step(world, 1.0F / 60.0F);
        CHECK(contacts().grounded && near(world.find(hero)->position.y, 320));
    }

    // Walking across two adjacent floor tiles does not snag on the seam.
    World tiles;
    PhysicsSystem tiled(settings);
    add_solid(tiles, tiled, {{0, 320}, {32, 32}});
    add_solid(tiles, tiled, {{32, 320}, {32, 32}});
    const auto walker = add_character(tiles, tiled, {10, 320});
    for (int i = 0; i < 20; ++i) {
        tiles.get<Body>(walker)->velocity.x = 60;
        tiled.step(tiles, 1.0F / 60.0F);
        CHECK(!tiles.get<ContactState>(walker)->wall_right);
        CHECK(near(tiles.find(walker)->position.y, 320));
    }
    CHECK(near(tiles.find(walker)->position.x, 30));

    // Ceiling: an upward jump into a low ceiling stops flush and cancels rise.
    add_solid(world, physics, {{0, 260}, {400, 20}}); // Bottom face at y = 280.
    world.get<Body>(hero)->velocity.y = -600;
    physics.step(world, 1.0F / 60.0F); // Would rise 10 px; head is at 288 -> 278.
    CHECK(contacts().ceiling && !contacts().grounded);
    CHECK(near(world.find(hero)->position.y, 312)); // Head flush with y = 280.
    CHECK(world.get<Body>(hero)->velocity.y == 0);
    CHECK(floor != invalid_entity);
}

void test_grounded_probe_and_edges() {
    World world;
    PhysicsSystem physics; // No gravity: grounding must come from the probe.
    const auto left = add_solid(world, physics, {{0, 100}, {50, 10}});
    const auto right = add_solid(world, physics, {{50, 100}, {50, 10}});
    const auto resting = add_character(world, physics, {20, 100});
    const auto straddling = add_character(world, physics, {50, 100});
    const auto hovering = add_character(world, physics, {20, 99.9F});
    const auto rising = add_character(world, physics, {80, 100}, Body{{0, -1}, {}});
    physics.step(world, 1.0F / 60.0F);
    CHECK(world.get<ContactState>(resting)->grounded);
    CHECK(world.get<ContactState>(resting)->ground == left);
    CHECK(world.get<ContactState>(straddling)->ground == left); // Lowest ID of two supports.
    CHECK(!world.get<ContactState>(hovering)->grounded);
    CHECK(world.get<ContactState>(hovering)->ground == invalid_entity);
    CHECK(!world.get<ContactState>(rising)->grounded);

    // Removing the support clears grounded state on the next step.
    CHECK(world.destroy(left));
    physics.step(world, 1.0F / 60.0F);
    CHECK(!world.get<ContactState>(resting)->grounded);
    CHECK(world.get<ContactState>(straddling)->ground == right);

    // Edge contact: a body whose side only touches the platform's side, with
    // its feet level with the top, is not supported and falls past the edge.
    World edge_world;
    PhysicsSettings settings;
    settings.gravity = {0, 900};
    PhysicsSystem edge_physics(settings);
    add_solid(edge_world, edge_physics, {{108, 100}, {50, 10}});
    const auto at_edge = add_character(edge_world, edge_physics, {100, 100}); // Right edge at 108.
    edge_physics.step(edge_world, 1.0F / 60.0F);
    CHECK(!edge_world.get<ContactState>(at_edge)->grounded);
    for (int i = 0; i < 10; ++i) edge_physics.step(edge_world, 1.0F / 60.0F);
    CHECK(edge_world.find(at_edge)->position.y > 101);
    CHECK(!edge_world.get<ContactState>(at_edge)->wall_right);

    // One pixel of overlap past the edge is enough to stand on.
    World ledge_world;
    PhysicsSystem ledge_physics(settings);
    add_solid(ledge_world, ledge_physics, {{108, 100}, {50, 10}});
    const auto on_ledge = add_character(ledge_world, ledge_physics, {101, 100});
    for (int i = 0; i < 10; ++i) ledge_physics.step(ledge_world, 1.0F / 60.0F);
    CHECK(ledge_world.get<ContactState>(on_ledge)->grounded);
    CHECK(near(ledge_world.find(on_ledge)->position.y, 100));
}

void test_layer_filtering_and_queries() {
    constexpr CollisionLayers terrain = 1U << 1U;
    constexpr CollisionLayers player = 1U << 2U;
    constexpr CollisionLayers fire_barrier = 1U << 3U;
    World world;
    PhysicsSettings settings;
    settings.gravity = {0, 900};
    PhysicsSystem physics(settings);
    const auto floor = add_solid(world, physics, {{0, 100}, {200, 20}}, terrain);
    const auto barrier = add_solid(world, physics, {{50, 0}, {10, 100}}, fire_barrier);
    const auto hero = add_character(world, physics, {20, 100}, Body{{1200, 0}, {}});
    auto& collider = *world.get<Collider>(hero);
    collider.layer = player;
    collider.mask = terrain; // Ignores the barrier layer.

    physics.step(world, 1.0F / 60.0F); // Moves 20 px through the barrier at x = 50.
    CHECK(near(world.find(hero)->position.x, 40));
    world.get<Body>(hero)->velocity.x = 1200;
    physics.step(world, 1.0F / 60.0F);
    CHECK(near(world.find(hero)->position.x, 60)); // Inside the barrier, unblocked.
    CHECK(world.get<ContactState>(hero)->grounded && world.get<ContactState>(hero)->ground == floor);

    // The barrier must also accept the body's layer: a one-sided mask is not enough.
    collider.mask = layer_all;
    world.get<Collider>(barrier)->mask = terrain;
    world.find(hero)->position.x = 20;
    world.get<Body>(hero)->velocity.x = 2400;
    physics.step(world, 1.0F / 60.0F);
    CHECK(near(world.find(hero)->position.x, 60));
    world.get<Collider>(barrier)->mask = layer_all;
    world.find(hero)->position.x = 20;
    world.get<Body>(hero)->velocity.x = 2400;
    physics.step(world, 1.0F / 60.0F);
    CHECK(near(world.find(hero)->position.x, 42) && world.get<ContactState>(hero)->wall_right);

    // Queries filter by layer, exclude touching edges and skip destroyed entities.
    const auto everything = physics.query(world, {{0, 0}, {200, 200}});
    CHECK(everything.size() == 3 && everything[0] == floor && everything[2] == hero);
    CHECK(physics.query(world, {{0, 0}, {200, 200}}, terrain).size() == 1);
    CHECK(physics.query(world, {{60, 0}, {5, 5}}).empty()); // Touches barrier's right edge.
    CHECK(physics.query(world, {{0, 0}, {0, 0}}).empty());
    CHECK(world.destroy(barrier));
    CHECK(physics.query(world, {{0, 0}, {200, 200}}, fire_barrier).empty());
}

EntityId add_trigger(World& world, PhysicsSystem& physics, Aabb box, CollisionLayers layer = layer_default) {
    const auto id = world.create({box.position});
    world.add<Collider>(id, Collider{{}, box.size, layer, layer_all, true});
    physics.add(world, id);
    return id;
}

bool has_event(const PhysicsSystem& physics, ContactPhase phase, EntityId body, EntityId trigger) {
    for (const auto& event : physics.events())
        if (event == CollisionEvent{phase, body, trigger}) return true;
    return false;
}

void test_trigger_events() {
    World world;
    PhysicsSystem physics;
    const auto exit = add_trigger(world, physics, {{40, 60}, {20, 40}});
    const auto hero = add_character(world, physics, {0, 100}, Body{{1200, 0}, {}}); // 20 px/tick.
    const auto tick = [&] { physics.step(world, 1.0F / 60.0F); };

    tick(); // Right edge 8 -> 28: not yet inside.
    CHECK(physics.events().empty());
    tick(); // Right edge 48: entered; triggers never block movement.
    CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::enter, hero, exit));
    CHECK(near(world.find(hero)->position.x, 40) && !world.get<ContactState>(hero)->wall_right);
    tick(); // Box 52..68 still overlaps 40..60.
    CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::stay, hero, exit));
    tick(); // Box 72..88: left.
    CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::leave, hero, exit));
    tick();
    CHECK(physics.events().empty());

    // A zero-length step clears events but remembers the overlap.
    world.find(hero)->position.x = 50;
    world.get<Body>(hero)->velocity = {};
    tick();
    CHECK(has_event(physics, ContactPhase::enter, hero, exit));
    physics.step(world, 0);
    CHECK(physics.events().empty());
    tick();
    CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::stay, hero, exit));

    // Destroying the trigger (e.g. a collected item) reports leave once.
    CHECK(world.destroy(exit));
    tick();
    CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::leave, hero, exit));
    tick();
    CHECK(physics.events().empty());
}

void test_trigger_filtering_and_moving_triggers() {
    constexpr CollisionLayers players = 1U << 1U;
    constexpr CollisionLayers sensors = 1U << 2U;
    World world;
    PhysicsSystem physics;
    const auto wall = add_solid(world, physics, {{100, 0}, {10, 200}});
    const auto plate = add_trigger(world, physics, {{0, 90}, {200, 10}}, sensors);
    const auto hidden = add_trigger(world, physics, {{0, 90}, {200, 10}}, sensors);
    world.get<Collider>(hidden)->mask = layer_default; // Ignores the player layer.
    const auto a = add_character(world, physics, {20, 100});
    const auto b = add_character(world, physics, {60, 100});
    world.get<Collider>(a)->layer = players;
    world.get<Collider>(b)->layer = players;
    // A moving hazard: trigger + Body moves straight through the solid wall.
    const auto hazard = world.create({{90, 50}});
    world.add<Body>(hazard, Body{{600, 0}, {}});
    world.add<Collider>(hazard, Collider{{}, {8, 8}, sensors, layer_all, true});
    physics.add(world, hazard);

    physics.step(world, 1.0F / 60.0F);
    const std::vector<CollisionEvent> expected{
        {ContactPhase::enter, a, plate}, {ContactPhase::enter, b, plate}};
    CHECK(physics.events() == expected); // Ordered by (body, trigger); hidden filtered out.
    for (int i = 0; i < 3; ++i) physics.step(world, 1.0F / 60.0F);
    CHECK(world.find(hazard)->position.x > 110); // Passed the wall at x = 100..110.
    CHECK(world.get<ContactState>(hazard) == nullptr); // Triggers get no ContactState.

    // Removing a body from physics (or making it a trigger) ends its contacts.
    CHECK(physics.remove(a));
    world.get<Collider>(b)->trigger = true;
    physics.step(world, 1.0F / 60.0F);
    const std::vector<CollisionEvent> ended{
        {ContactPhase::leave, a, plate}, {ContactPhase::leave, b, plate}};
    CHECK(physics.events() == ended);
    CHECK(wall != invalid_entity);
}

void test_contact_cleanup_after_restart() {
    World world;
    PhysicsSystem physics;
    const auto hazard = add_trigger(world, physics, {{100, 80}, {40, 20}});
    const auto exit = add_trigger(world, physics, {{0, 80}, {20, 20}}); // Covers the spawn.
    const auto hero = add_character(world, physics, {120, 100});
    physics.step(world, 1.0F / 60.0F);
    CHECK(has_event(physics, ContactPhase::enter, hero, hazard));

    // Restart: the game teleports the hero to spawn and resets physics contacts.
    world.find(hero)->position = {200, 100};
    physics.reset();
    CHECK(physics.events().empty());
    physics.step(world, 1.0F / 60.0F);
    CHECK(physics.events().empty()); // No stale leave for the old hazard.

    // Restarting while overlapping produces a fresh enter, not a stay.
    world.find(hero)->position = {10, 100};
    physics.step(world, 1.0F / 60.0F);
    CHECK(has_event(physics, ContactPhase::enter, hero, exit));
    for (int round = 0; round < 3; ++round) {
        physics.reset();
        physics.step(world, 1.0F / 60.0F);
        CHECK(physics.events().size() == 1 && has_event(physics, ContactPhase::enter, hero, exit));
    }
    CHECK(physics.size() == 3); // Restart keeps registered entities.
}

} // namespace

int main() {
    return run_test([] {
        test_integration_and_overlap();
        test_collider_bounds_and_layers();
        test_settings_validation();
        test_registration_and_removal();
        test_step_integration();
        test_invalid_components_rejected_before_mutation();
        test_walls();
        test_floors_and_ceilings();
        test_grounded_probe_and_edges();
        test_layer_filtering_and_queries();
        test_trigger_events();
        test_trigger_filtering_and_moving_triggers();
        test_contact_cleanup_after_restart();
    });
}
