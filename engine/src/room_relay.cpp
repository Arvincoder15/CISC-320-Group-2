#include "engine/room_relay.hpp"
#include "room_wire.hpp"

#ifdef _WIN32
#define NOMINMAX
#endif
#include <enet/enet.h>

#include <chrono>
#include <random>
#include <stdexcept>
#include <unordered_map>

namespace engine::net {
bool enet_ready();

namespace {
using room_wire::Kind;
using Clock = std::chrono::steady_clock;
constexpr std::size_t max_peers = 64;
constexpr std::size_t queue_limit = 64 * 1024;
constexpr auto request_timeout = std::chrono::seconds(10);
constexpr auto room_timeout = std::chrono::minutes(10);

void send_control(ENetPeer* peer, Kind kind, std::string_view text = {}) {
    const auto bytes = room_wire::packet(kind, text);
    auto* packet = enet_packet_create(bytes.data(), bytes.size(), ENET_PACKET_FLAG_RELIABLE);
    if (packet && enet_peer_send(peer, 0, packet) != 0) enet_packet_destroy(packet);
}
}

struct RoomRelay::Impl {
    struct Member {
        Clock::time_point deadline{Clock::now() + request_timeout};
        std::string code;
        ENetPeer* partner{};
        bool closing{};
    };

    explicit Impl(std::uint16_t port)
        : server(nullptr, enet_host_destroy) {
        if (!enet_ready()) throw std::runtime_error("Could not initialize ENet");
        ENetAddress address{ENET_HOST_ANY, port};
        server.reset(enet_host_create(&address, max_peers, 2, 0, 0));
        if (!server) throw std::runtime_error("Could not bind relay UDP port " + std::to_string(port));
        server->maximumPacketSize = room_wire::packet_limit;
        server->maximumWaitingData = queue_limit;
    }

    std::string new_code() {
        std::uniform_int_distribution<std::size_t> pick(0, room_wire::alphabet.size() - 1);
        std::string code(room_wire::code_length, ' ');
        do {
            for (char& c : code) c = room_wire::alphabet[pick(random)];
        } while (rooms.contains(code));
        return code;
    }

    void end(ENetPeer* peer, std::string_view reason, bool disconnected = false) {
        const auto found = members.find(peer);
        if (found == members.end()) return;
        auto& member = found->second;
        if (!member.code.empty()) rooms.erase(member.code);
        auto* partner = std::exchange(member.partner, nullptr);
        member.code.clear();
        member.closing = true;
        if (disconnected) {
            members.erase(found);
        } else {
            send_control(peer, Kind::rejected, reason);
            enet_peer_disconnect_later(peer, 0);
        }
        if (partner) {
            members.at(partner).partner = nullptr;
            end(partner, "The other player left the room");
        }
    }

    void receive(const ENetEvent& event) {
        const auto found = members.find(event.peer);
        const auto size = event.packet->dataLength;
        if (found == members.end() || found->second.closing || size == 0 ||
            size > room_wire::packet_limit || event.channelID > 1) return;

        auto& member = found->second;
        const auto* data = reinterpret_cast<const std::byte*>(event.packet->data);
        const auto kind = static_cast<Kind>(*data);

        if (kind == Kind::data) {
            if (member.partner && size > 1) enet_peer_send(member.partner, event.channelID, event.packet);
            return;
        }
        if (event.channelID != 0 || member.partner || !member.code.empty()) return;

        if (kind == Kind::create && size == 1) {
            member.code = new_code();
            member.deadline = Clock::now() + room_timeout;
            rooms.emplace(member.code, event.peer);
            send_control(event.peer, Kind::created, member.code);
            return;
        }
        if (kind == Kind::join && size == room_wire::code_length + 1) {
            const auto code = room_wire::normalize_code(room_wire::read_text({data + 1, size - 1}));
            const auto room = rooms.find(code);
            if (room == rooms.end()) {
                end(event.peer, "Room code is invalid, expired, or already in use");
                return;
            }
            auto* host = room->second;
            rooms.erase(room);
            auto& host_member = members.at(host);
            host_member.code.clear();
            host_member.partner = event.peer;
            member.partner = host;
            send_control(host, Kind::paired);
            send_control(event.peer, Kind::paired);
        } else {
            end(event.peer, "Invalid room request");
        }
    }

    void poll() {
        ENetEvent event{};
        for (int i = 0; i < 256; ++i) {
            const int result = enet_host_service(server.get(), &event, 0);
            if (result < 0) throw std::runtime_error("Could not poll relay socket");
            if (result == 0) break;
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                members.emplace(event.peer, Member{});
                enet_peer_timeout(event.peer, 8, 3000, 8000);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                end(event.peer, {}, true);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                receive(event);
                // A successful forward transferred the packet to ENet's send queue.
                if (event.packet->referenceCount == 0) enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }

        const auto now = Clock::now();
        for (const auto& [peer, member] : members) {
            if (!member.closing && !member.partner && now >= member.deadline)
                end(peer, member.code.empty() ? "Room request timed out" : "Room expired. Create a new room");
        }
        enet_host_flush(server.get());
    }

    std::unique_ptr<ENetHost, decltype(&enet_host_destroy)> server;
    std::random_device random;
    std::unordered_map<ENetPeer*, Member> members;
    std::unordered_map<std::string, ENetPeer*> rooms;
};

RoomRelay::RoomRelay(std::uint16_t port) : impl_(std::make_unique<Impl>(port)) {}
RoomRelay::~RoomRelay() = default;
std::uint16_t RoomRelay::port() const { return impl_->server->address.port; }
void RoomRelay::poll() { impl_->poll(); }

}
