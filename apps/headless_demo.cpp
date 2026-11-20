#include "engine/infrastructure.hpp"
#include "engine/networking.hpp"
#include "engine/rendering.hpp"
#include "engine/tools.hpp"
#include "elemental_coop/game.hpp"
#include <array>
#include <charconv>
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>
namespace {
int run_local_demo() {
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
    return 0;
}

// Two-process connection check (NET-05): logs session state changes until the connection ends.
int run_network(engine::net::Session& session, bool started) {
    using engine::net::SessionState;
    std::cout << std::unitbuf; // flush each line so logs survive a killed process or a pipe
    engine::Logger logger(std::cout);
    if (!started) {
        logger.write(engine::LogLevel::error, session.last_error());
        return 5;
    }
    if (session.state() == SessionState::hosting)
        logger.write(engine::LogLevel::info, "Hosting on port " + std::to_string(session.port()) + "; waiting for a player");
    bool was_connected = false;
    for (;;) {
        session.poll();
        const auto state = session.state();
        if (state == SessionState::connected && !was_connected) {
            logger.write(engine::LogLevel::info, "Connected");
            was_connected = true;
        }
        if (state == SessionState::failed || state == SessionState::disconnected) {
            logger.write(engine::LogLevel::warning, session.last_error());
            return was_connected ? 0 : 5;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool parse_port(std::string_view text, std::uint16_t& port) {
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), port);
    return error == std::errc{} && end == text.data() + text.size();
}
}

int main(int argc, char** argv) {
    if (argc == 1) return run_local_demo();
    const std::string_view mode = argv[1];
    std::uint16_t port = 0;
    auto session = engine::net::make_enet_session();
    if (mode == "--host" && argc == 3 && parse_port(argv[2], port)) return run_network(*session, session->host(port));
    if (mode == "--join" && argc == 4 && parse_port(argv[3], port) && port != 0)
        return run_network(*session, session->join(argv[2], port));
    std::cerr << "Usage: engine_demo                      (scripted local demo)\n"
                 "       engine_demo --host <port>        (0 = any free port)\n"
                 "       engine_demo --join <address> <port>\n";
    return 64;
}
