#include "engine/infrastructure.hpp"
#include "engine/networking.hpp"
#include "engine/rendering.hpp"
#include "engine/tools.hpp"
#include "elemental_coop/game.hpp"
#include <array>
#include <iostream>
int main() {
    engine::Logger logger(std::cout);
    logger.write(engine::LogLevel::info, "Two-player co-op headless starter: scripted input, no graphics or sockets");
    engine::LevelDocument level{1, {{"fire_player", {64, 320}}, {"water_player", {128, 320}}}};
    if (!engine::validate(level).empty()) return 1;
    engine::ResourceCache<std::string> resources([](const std::string& key) {
        return std::make_shared<std::string>(key); // Metadata only, not texture loading.
    });
    const auto fire_asset = resources.load("placeholder/fire_player");
    const auto water_asset = resources.load("placeholder/water_player");
    elemental_coop::Game game;
    std::array<engine::InputBuffer, 2> inputs;
    inputs[0].set_move_x(1);
    inputs[1].set_move_x(-1);
    engine::RecordingRenderer renderer;
    engine::LoopbackTransport transport;
    for (std::uint64_t tick = 0; tick < 120; ++tick) {
        if (tick == 0) inputs[0].press_primary();
        if (tick == 30) inputs[1].press_primary();
        game.tick({inputs[0].consume(), inputs[1].consume()}, 1.0F / 60.0F);
        std::array<engine::SpriteDraw, 2> draws;
        for (std::size_t i = 0; i < 2; ++i) {
            const auto entity = game.player(i).entity;
            const auto position = game.world().find(entity)->position;
            draws[i] = {entity, position, i == 0 ? *fire_asset : *water_asset};
            if (!transport.send({tick, entity, position})) return 2;
            if (!transport.receive()) return 3;
        }
        renderer.draw(draws);
    }
    // Scripted contacts demonstrate game rules, not real collision detection.
    game.touch_hazard(0, elemental_coop::Hazard::lava);
    game.touch_hazard(1, elemental_coop::Hazard::water);
    game.set_exit_contact(0, true);
    game.set_exit_contact(1, true);
    if (game.state() != elemental_coop::RoundState::won) return 4;
    std::cout << "Simulated 120 ticks; players=" << game.world().size()
              << "; rendered commands=" << renderer.last_frame().size()
              << "; cached resources=" << resources.size() << "; cooperative exit reached\n";
}
