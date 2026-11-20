#pragma once

#include "engine/networking.hpp"

namespace engine::net {

inline constexpr std::uint16_t room_relay_port = 7777;

// In relay mode host() creates a room and join() takes a room code as its address.
// The endpoint comes from this factory. The host learns its code through room_code().
// An endpoint accepts a hostname or IPv4 address with an optional :port.
// An invalid endpoint throws std::invalid_argument.
std::unique_ptr<Session> make_room_session(std::string endpoint);
std::unique_ptr<Session> make_room_session(std::string relay_address, std::uint16_t relay_port);

// A single-threaded ENet relay. Poll regularly from the relay process.
class RoomRelay {
public:
    explicit RoomRelay(std::uint16_t port);
    ~RoomRelay();
    RoomRelay(const RoomRelay&) = delete;
    RoomRelay& operator=(const RoomRelay&) = delete;

    std::uint16_t port() const;
    void poll();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
