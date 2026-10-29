#include "engine/architecture.hpp"
#include "test_support.hpp"
#include <algorithm>

int main()
{
    struct Health
    {
        int hp{0};
    };
    struct Position
    {
        float x{0};
        float y{0};
    };

    return run_test([]
                    {
                        {
                            engine::World world;
                            const auto a = world.create({{3, 4}});
                            const auto b = world.create();
                            CHECK(a != b && a != engine::invalid_entity);
                            CHECK(world.find(a)->position.x == 3);
                            CHECK(world.alive(a) == true);
                            CHECK(world.destroy(a));
                            CHECK(world.alive(a) == false);
                            CHECK(!world.destroy(a));
                            CHECK(world.find(a) == nullptr);
                            CHECK(world.size() == 1);
                            CHECK(world.create() > b);

                            // Other alive() tests
                            CHECK(world.alive(engine::invalid_entity) == false);
                            CHECK(world.alive(99999) == false);
                            CHECK(world.alive(b) == true);
                        };


                        { // Destroy loop tests
                            engine::World world;
                            const auto player = world.create();
                            const auto enemy = world.create();

                            world.add<Health>(player, Health{100});
                            world.add<Position>(player, Position{});
                            world.add<Health>(enemy, Health{100});
                            world.add<Position>(enemy, Position{3.0});

                            CHECK(world.count<Health>() == 2); //Should probably be 1, need to double check this later
                            CHECK(world.count<Position>() == 2);

                            world.destroy(player);
                            //CHECK(world.get(player) == nullptr);
                        }; }

    );
}
