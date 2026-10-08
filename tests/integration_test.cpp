#include "elemental_coop/game.hpp"
#include "test_support.hpp"
#include "engine/rendering.hpp"
#include "engine/networking.hpp"
#include <array>
int main() { return run_test([] {
    std::array<engine::InputBuffer, 2> input;
    input[0].set_move_x(1);
    input[0].press_primary();
    input[1].set_move_x(-1);
    elemental_coop::Game game;
    game.tick({input[0].consume(), input[1].consume()}, 1.0F / 60.0F);
    engine::RecordingRenderer renderer;
    engine::LoopbackTransport transport;
    std::array<engine::SpriteDraw, 2> draws;
    for (std::size_t i = 0; i < 2; ++i) {
        const auto entity = game.player(i).entity;
        const auto position = game.world().find(entity)->position;
        draws[i] = {entity, position, i == 0 ? "fire_player" : "water_player"};
        CHECK(transport.send({1, entity, position}));
    }
    renderer.draw(draws);
    CHECK(renderer.last_frame().size() == 2);
    CHECK(near(draws[0].position.x, 67) && near(draws[0].position.y, 314.25F));
    CHECK(near(draws[1].position.x, 125) && near(draws[1].position.y, 320));
    for (const auto& draw : renderer.last_frame()) {
        const auto received = transport.receive();
        CHECK(received && received->entity == draw.entity);
        CHECK(near(received->position.x, draw.position.x) && near(received->position.y, draw.position.y));
    }
    CHECK(!transport.receive());
    CHECK(!input[0].consume().primary_pressed && !input[1].consume().primary_pressed);
    game.set_exit_contact(0, true);
    CHECK(game.state() == elemental_coop::RoundState::playing);
    game.set_exit_contact(1, true);
    CHECK(game.state() == elemental_coop::RoundState::won);
}); }
