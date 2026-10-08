#include "engine/architecture.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::World world;
    const auto a = world.create({{3, 4}});
    const auto b = world.create();
    CHECK(a != b && a != engine::invalid_entity);
    CHECK(world.find(a)->position.x == 3);
    CHECK(world.destroy(a));
    CHECK(!world.destroy(a));
    CHECK(world.find(a) == nullptr);
    CHECK(world.size() == 1);
    CHECK(world.create() > b);
}); }
