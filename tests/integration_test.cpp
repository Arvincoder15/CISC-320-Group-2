#include "flappy/game.hpp"
#include "test_support.hpp"
#include "engine/ai.hpp"
#include "engine/rendering.hpp"
#include "engine/networking.hpp"
#include <array>
int main() { return run_test([] {
    engine::InputBuffer input;
    engine::VerticalSteering ai;
    if (ai.request_upward_action({10, 0, 0})) input.press_primary();
    flappy::Game game;
    game.tick(input.consume(), 1.0F / 60.0F);
    const auto position = game.world().find(game.player())->position;
    CHECK(near(position.y, -4.75F));
    engine::RecordingRenderer renderer;
    renderer.draw(std::array{engine::SpriteDraw{game.player(), position, "player"}});
    engine::LoopbackTransport transport;
    CHECK(transport.send({1, game.player(), position}));
    const auto received = transport.receive();
    CHECK(received && received->entity == game.player());
    CHECK(near(received->position.y, renderer.last_frame()[0].position.y));
    CHECK(!input.consume().primary_pressed);
}); }
