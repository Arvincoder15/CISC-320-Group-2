#include "engine/ai.hpp"
#include "engine/infrastructure.hpp"
#include "engine/networking.hpp"
#include "engine/rendering.hpp"
#include "engine/tools.hpp"
#include "flappy/game.hpp"
#include <array>
#include <iostream>
int main() {
    engine::Logger logger(std::cout);
    logger.write(engine::LogLevel::info, "Headless starter: no graphics or real network connection");
    engine::LevelDocument level{1, {{"player", {0, 0}}}};
    if (!engine::validate(level).empty()) return 1;
    engine::ResourceCache<std::string> resources([](const std::string& key) {
        return std::make_shared<std::string>(key); // Placeholder metadata, not texture loading.
    });
    auto asset = resources.load("placeholder/player");
    flappy::Game game;
    engine::InputBuffer input;
    engine::VerticalSteering ai;
    engine::RecordingRenderer renderer;
    engine::LoopbackTransport transport;
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        const auto y = game.world().find(game.player())->position.y;
        if (tick == 0 || ai.request_upward_action({y, 0, 10})) input.press_primary();
        game.tick(input.consume(), 1.0F / 60.0F);
        const auto position = game.world().find(game.player())->position;
        renderer.draw(std::array{engine::SpriteDraw{game.player(), position, *asset}});
        if (!transport.send({tick, game.player(), position})) return 2;
        if (!transport.receive()) return 3;
    }
    std::cout << "Simulated 120 ticks; entities=" << game.world().size()
              << "; rendered commands=" << renderer.last_frame().size()
              << "; cached resources=" << resources.size() << '\n';
}
