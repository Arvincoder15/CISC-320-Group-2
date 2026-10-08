#include "flappy/game.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    flappy::Game game;
    game.tick({true, false}, 1.0F / 60.0F);
    CHECK(game.world().find(game.player())->position.y < 0);
    const auto before_quit = game.world().find(game.player())->position.y;
    game.tick({false, true}, 1.0F / 60.0F);
    game.tick({}, 1.0F);
    CHECK(!game.running());
    CHECK(game.world().find(game.player())->position.y == before_quit);
    CHECK(throws_as<std::invalid_argument>([] { flappy::Game invalid({900, 0}); }));
    CHECK(throws_as<std::invalid_argument>([&] { game.tick({}, -1); }));
}); }
