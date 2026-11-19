#include "engine/rendering.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::RecordingRenderer renderer;
    std::vector<engine::SpriteDraw> draws{{1, {2, 3}, "sprite"}};
    renderer.draw(draws);
    draws[0].position.x = 99;
    CHECK(renderer.last_frame()[0].position.x == 2);
    renderer.draw({});
    CHECK(renderer.last_frame().empty());
    engine::Camera camera;
    camera.position={100,50}; camera.zoom=2;
    auto screen=camera.world_to_screen({110,75});
    CHECK(near(screen.x,20) && near(screen.y,50));
    auto world=camera.screen_to_world(screen);
    CHECK(near(world.x,110) && near(world.y,75));
    camera.follow({400,300});
    screen=camera.world_to_screen({400,300});
    CHECK(near(screen.x,480) && near(screen.y,270));
    camera.zoom=0;
    CHECK(throws_as<std::invalid_argument>([&]{camera.validate();}));
    engine::SpriteAnimation animation(3,5,{16,24},10);
    animation.advance(0.31);
    CHECK(animation.frame()==3);
    CHECK(near(animation.source().x,0) && near(animation.source().y,24));
    animation.advance(0.21);
    CHECK(animation.frame()==0);
    CHECK(throws_as<std::invalid_argument>([&]{animation.advance(-1);}));
    engine::SpriteAnimation once(3,5,{16,24},10,false);
    once.advance(1000000);
    CHECK(once.frame()==4);
    once.reset(); CHECK(once.frame()==0);
    const std::vector<unsigned> tiles{0,1,4,2};
    auto tile_commands=engine::tile_draws(tiles,2,{16,24},2,"tiles",{5,10},-2);
    CHECK(tile_commands.size()==3);
    CHECK(near(tile_commands[0].position.x,21));
    CHECK(near(tile_commands[1].position.y,34));
    CHECK(near(tile_commands[1].source->x,16) && near(tile_commands[1].source->y,24));
    CHECK(tile_commands[1].layer==-2);
    CHECK(throws_as<std::invalid_argument>([&]{engine::tile_draws(tiles,3,{16,24},2,"tiles");}));
    auto text=engine::text_draws("A\nB","font",{10,20},{8,12},2);
    CHECK(text.size()==2 && text[0].screen_space);
    CHECK(near(text[1].position.x,10) && near(text[1].position.y,44));
    CHECK(near(text[0].source->x,8) && near(text[0].source->y,24));
}); }
