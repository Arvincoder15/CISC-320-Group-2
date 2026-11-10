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
    });
}
