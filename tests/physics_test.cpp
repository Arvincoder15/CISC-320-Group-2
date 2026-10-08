#include "engine/physics.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::Transform transform;
    engine::Body body{{0, 0}, {0, 10}};
    engine::integrate(transform, body, 0.5F);
    CHECK(near(body.velocity.y, 5));
    CHECK(near(transform.position.y, 2.5F));
    engine::integrate(transform, body, 0);
    CHECK(near(transform.position.y, 2.5F));
    CHECK(throws_as<std::invalid_argument>([&] { engine::integrate(transform, body, -1); }));
    CHECK(engine::overlaps({{0, 0}, {10, 10}}, {{9, 9}, {3, 3}}));
    CHECK(!engine::overlaps({{0, 0}, {10, 10}}, {{10, 0}, {3, 3}}));
    CHECK(!engine::overlaps({{0, 0}, {0, 10}}, {{0, 0}, {3, 3}}));
}); }
