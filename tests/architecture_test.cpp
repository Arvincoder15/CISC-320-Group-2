#include "engine/architecture.hpp"
#include "test_support.hpp"
int main()
{
    return run_test([]
                    {{
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

    //Other alive() tests
    CHECK(world.alive(engine::invalid_entity)== false); 
    CHECK(world.alive(99999)== false); 
    CHECK(world.alive(b)== false); };

    { //Component add/get/remove tests 
    engine::World world;
    const auto player = world.create();
    CHECK(world.get_health(player) == nullptr);
    world.add_health(player, engine::TempHealth{100});
    CHECK(world.get_health(player) != nullptr);
    CHECK( world.get_health(player)->hp == 100);
    world.get_health(player)->hp = 70; 
    CHECK(world.get_health(player)->hp == 70);
    const engine::World& view = world;
    CHECK(view.get_health(player)->hp ==70);
    CHECK(world.remove_health(player)==true);
    CHECK(world.remove_health(player)==false);
    world.add_health(player, engine::TempHealth{100});
    world.add_health(player, engine::TempHealth{50});
    CHECK( world.get_health(player)->hp ==50);



    
    }; }

    );
}
