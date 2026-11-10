#include "engine/physics.hpp"
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

} // namespace

int main() {
    return run_test([] {
        test_integration_and_overlap();
        test_collider_bounds_and_layers();
        test_settings_validation();
    });
}
