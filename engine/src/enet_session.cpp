#include "engine/networking.hpp"
#ifdef _WIN32
#define NOMINMAX
#endif
#include <enet/enet.h>
#include <chrono>
#include <cstdlib>
namespace engine::net {
namespace {
constexpr std::size_t max_packet_bytes = 512;          // largest co-op message is under 50 bytes
constexpr auto connect_timeout = std::chrono::seconds(5);

bool enet_ready() {
    static const bool ready = [] {
        if (enet_initialize() != 0) return false;
        std::atexit(enet_deinitialize);
        return true;
    }();
    return ready;
}

class EnetSession final : public Session {
public:
    ~EnetSession() override { close(); }
    bool host(std::uint16_t port) override {
        if (!start()) return false;
        ENetAddress address{};
        address.host = ENET_HOST_ANY;
        address.port = port;
        host_ = enet_host_create(&address, 1, 2, 0, 0); // one peer: a third player is refused
        if (!host_) return fail("Could not open port " + std::to_string(port));
        port_ = host_->address.port;
        state_ = SessionState::hosting;
        return true;
    }
    bool join(const std::string& address, std::uint16_t port) override {
        if (!start()) return false;
        ENetAddress target{};
        if (enet_address_set_host(&target, address.c_str()) != 0) return fail("Unknown host address: " + address);
        target.port = port;
        host_ = enet_host_create(nullptr, 1, 2, 0, 0);
        if (!host_) return fail("Could not create network client");
        peer_ = enet_host_connect(host_, &target, 2, 0);
        if (!peer_) return fail("Could not start connection");
        state_ = SessionState::connecting;
        deadline_ = std::chrono::steady_clock::now() + connect_timeout;
        return true;
    }
    bool send(Channel channel, std::span<const std::byte> bytes) override {
        if (state_ != SessionState::connected) return false;
        const auto flags = channel == Channel::reliable ? ENET_PACKET_FLAG_RELIABLE : 0u;
        ENetPacket* packet = enet_packet_create(bytes.data(), bytes.size(), flags);
        if (!packet) return false;
        if (enet_peer_send(peer_, static_cast<enet_uint8>(channel), packet) != 0) {
            if (packet->referenceCount == 0) enet_packet_destroy(packet);
            return false;
        }
        return true;
    }
    std::vector<Event> poll() override {
        std::vector<Event> events;
        if (!host_) return events;
        ENetEvent event;
        while (host_ && enet_host_service(host_, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                peer_ = event.peer;
                enet_peer_timeout(peer_, 0, 1000, 3000); // report a vanished peer within 3 s (NFR-17)
                state_ = SessionState::connected;
                events.push_back({Event::Kind::connected, {}});
                break;
            case ENET_EVENT_TYPE_RECEIVE: {
                const auto* data = reinterpret_cast<const std::byte*>(event.packet->data);
                if (event.peer == peer_ && event.packet->dataLength <= max_packet_bytes)
                    events.push_back({Event::Kind::message, {data, data + event.packet->dataLength}});
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer != peer_) break;
                peer_ = nullptr;
                if (state_ == SessionState::connecting) {
                    state_ = SessionState::failed;
                    error_ = "Could not reach host";
                } else {
                    state_ = SessionState::disconnected;
                    error_ = "Connection lost";
                    events.push_back({Event::Kind::disconnected, {}});
                }
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }
        if (state_ == SessionState::connecting && std::chrono::steady_clock::now() > deadline_) {
            enet_peer_reset(peer_);
            peer_ = nullptr;
            state_ = SessionState::failed;
            error_ = "Could not reach host";
        }
        if (host_) enet_host_flush(host_);
        return events;
    }
    SessionState state() const override { return state_; }
    const std::string& last_error() const override { return error_; }
    std::uint16_t port() const override { return port_; }
    void close() override {
        if (peer_ && state_ == SessionState::connected) enet_peer_disconnect_now(peer_, 0); // tell the peer at once
        if (host_) enet_host_destroy(host_);
        host_ = nullptr;
        peer_ = nullptr;
        port_ = 0;
        state_ = SessionState::idle;
    }
private:
    bool start() {
        if (host_) return fail("Session already in use");
        error_.clear();
        if (!enet_ready()) return fail("Network library failed to start");
        return true;
    }
    bool fail(std::string message) {
        if (host_) enet_host_destroy(host_);
        host_ = nullptr;
        peer_ = nullptr;
        state_ = SessionState::failed;
        error_ = std::move(message);
        return false;
    }
    ENetHost* host_{nullptr};
    ENetPeer* peer_{nullptr};
    SessionState state_{SessionState::idle};
    std::string error_;
    std::uint16_t port_{0};
    std::chrono::steady_clock::time_point deadline_{};
};
}

std::unique_ptr<Session> make_enet_session() { return std::make_unique<EnetSession>(); }
}
