#include "elemental_coop/game.hpp"
#include "test_support.hpp"
#include <limits>
using namespace elemental_coop;
int main() { return run_test([] {
    Game game;
    const auto position = [&](std::size_t index) {
        return game.world().find(game.player(index).entity)->position;
    };
    CHECK(game.world().size() == 2);
    CHECK(game.player(0).entity != game.player(1).entity);
    CHECK(game.player(0).element == Element::fire && game.player(1).element == Element::water);
    game.tick({engine::InputFrame{1, true, false}, engine::InputFrame{-1, false, false}}, 1.0F / 60.0F);
    CHECK(near(position(0).x, 67) && near(position(1).x, 125));
    CHECK(near(position(0).y, 314.25F) && near(position(1).y, 320));
    CHECK(!game.player(0).grounded && game.player(1).grounded);
    // A second airborne press must not reset vertical velocity (no double jump).
    game.tick({engine::InputFrame{0, true, false}, {}}, 1.0F / 60.0F);
    CHECK(near(game.player(0).body.velocity.y, -330));
    for (int i = 0; i < 90; ++i) game.tick({}, 1.0F / 60.0F);
    CHECK(game.player(0).grounded && near(position(0).y, 320));
    game.tick({engine::InputFrame{0, true, false}, {}}, 1.0F / 60.0F);
    CHECK(!game.player(0).grounded);
    // All player/hazard combinations; either death loses the shared round.
    for (std::size_t player = 0; player < 2; ++player) {
        for (const auto hazard : {Hazard::lava, Hazard::water, Hazard::toxic}) {
            game.restart();
            game.touch_hazard(player, hazard);
            const bool safe = (player == 0 && hazard == Hazard::lava) || (player == 1 && hazard == Hazard::water);
            CHECK(game.player(player).alive == safe);
            CHECK(game.state() == (safe ? RoundState::playing : RoundState::lost));
        }
    }
    const auto stopped = position(0);
    game.tick({engine::InputFrame{1, true, false}, {}}, 0.1F);
    CHECK(position(0).x == stopped.x && position(0).y == stopped.y);
    game.set_exit_contact(0, true);
    game.set_exit_contact(1, true);
    CHECK(game.state() == RoundState::lost);
    // Restart restores both players and retains exactly two entities.
    for (int i = 0; i < 3; ++i) game.restart();
    CHECK(game.world().size() == 2 && game.state() == RoundState::playing);
    CHECK(game.player(0).alive && game.player(1).alive);
    CHECK(game.player(0).grounded && game.player(1).grounded);
    CHECK(near(position(0).x, 64) && near(position(1).x, 128));
    CHECK(game.player(0).body.velocity.y == 0 && !game.player(0).at_exit);
    game.set_exit_contact(0, true);
    game.set_exit_contact(1, false);
    CHECK(game.state() == RoundState::playing);
    // Contacts must be reported together for the current simulation tick.
    game.tick({}, 1.0F / 60.0F);
    game.set_exit_contact(1, true);
    CHECK(game.state() == RoundState::playing);
    game.set_exit_contact(0, true);
    CHECK(game.state() == RoundState::won);
    const auto completed = position(1);
    game.tick({engine::InputFrame{}, engine::InputFrame{1, true, false}}, 0.1F);
    CHECK(position(1).x == completed.x);
    game.restart();
    game.tick({engine::InputFrame{}, engine::InputFrame{0, false, true}}, 0.1F);
    CHECK(game.state() == RoundState::quit);
    game.restart();
    CHECK(throws_as<std::invalid_argument>([] { Game invalid({900, 0, 360, 320}); }));
    CHECK(throws_as<std::invalid_argument>([&] { game.tick({}, -1); }));
    CHECK(throws_as<std::invalid_argument>([&] { game.tick({}, std::numeric_limits<float>::quiet_NaN()); }));
    CHECK(throws_as<std::invalid_argument>([&] { game.tick({engine::InputFrame{1, true, false}, engine::InputFrame{2, false, false}}, 0.1F); }));
    CHECK(near(position(0).x, 64) && game.player(0).grounded); // Invalid frame was atomic.
    CHECK(throws_as<std::out_of_range>([&] { game.touch_hazard(2, Hazard::lava); }));
}); }
