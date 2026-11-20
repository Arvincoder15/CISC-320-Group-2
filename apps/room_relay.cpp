#include "engine/room_relay.hpp"

#include <charconv>
#include <chrono>
#include <iostream>
#include <string_view>
#include <thread>

int main(int argc, char** argv) {
    unsigned port = engine::net::room_relay_port;
    if (argc == 2) {
        const std::string_view value(argv[1]);
        const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), port);
        if (error != std::errc{} || end != value.data() + value.size() || port > 65535) argc = 3;
    }
    if (argc > 2) {
        std::cerr << "Usage: room_relay [udp-port]\n";
        return 1;
    }

    try {
        engine::net::RoomRelay relay(static_cast<std::uint16_t>(port));
        std::cout << "Elemental Co-op relay listening on UDP " << relay.port() << std::endl;
        for (;;) {
            relay.poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    } catch (const std::exception& error) {
        std::cerr << "Room relay: " << error.what() << '\n';
        return 1;
    }
}
