#include "engine/room_relay.hpp"
#include "room_wire.hpp"

#include <charconv>
#include <chrono>
#include <stdexcept>

namespace engine::net {
namespace {
using room_wire::Kind;
using Clock = std::chrono::steady_clock;

class RoomSession final : public Session {
public:
    RoomSession(std::string relay, std::uint16_t port)
        : relay_(std::move(relay)), relay_port_(port), transport_(make_enet_session()) {}

    bool host(std::uint16_t) override { return start(room_wire::packet(Kind::create)); }

    bool join(const std::string& address, std::uint16_t) override {
        if (active()) return false;
        const auto code = room_wire::normalize_code(address);
        if (code.empty()) return fail("Enter a valid 10-character room code");
        return start(room_wire::packet(Kind::join, code));
    }

    bool send(Channel channel, std::span<const std::byte> bytes) override {
        if (state_ != SessionState::connected || bytes.empty() || bytes.size() >= room_wire::packet_limit)
            return false;
        auto packet = room_wire::packet(Kind::data);
        packet.insert(packet.end(), bytes.begin(), bytes.end());
        return transport_->send(channel, packet);
    }

    std::vector<Event> poll() override {
        std::vector<Event> events;
        if (!active()) return events;

        for (auto& event : transport_->poll()) {
            if (event.kind == Event::Kind::connected) {
                if (!transport_->send(Channel::reliable, request_)) fail("Could not request a relay room");
            } else if (event.kind == Event::Kind::message && !event.payload.empty()) {
                handle(event, events);
            }
            if (!active()) break;
        }

        const auto transport_state = transport_->state();
        if (active() && (transport_state == SessionState::failed || transport_state == SessionState::disconnected)) {
            if (state_ == SessionState::connected || state_ == SessionState::hosting) {
                state_ = SessionState::disconnected;
                error_ = "Connection lost";
                events.push_back({Event::Kind::disconnected, {}});
            } else {
                fail("Could not reach the room relay");
            }
        }
        if (state_ == SessionState::connecting && Clock::now() > deadline_)
            fail("The room relay did not respond");
        return events;
    }

    SessionState state() const override { return state_; }
    const std::string& last_error() const override { return error_; }
    std::uint16_t port() const override { return 0; }
    std::string room_code() const override { return code_; }
    void close() override {
        transport_->close();
        code_.clear();
        error_.clear();
        state_ = SessionState::idle;
    }

private:
    bool active() const {
        return state_ == SessionState::connecting || state_ == SessionState::hosting
            || state_ == SessionState::connected;
    }

    bool fail(std::string error) {
        transport_->close();
        error_ = std::move(error);
        state_ = SessionState::failed;
        return false;
    }

    bool start(std::vector<std::byte> request) {
        if (active()) return false;
        close();
        if (!transport_->join(relay_, relay_port_)) return fail("Could not reach the room relay");
        request_ = std::move(request);
        state_ = SessionState::connecting;
        deadline_ = Clock::now() + std::chrono::seconds(8);
        return true;
    }

    void handle(Event& event, std::vector<Event>& output) {
        const auto kind = static_cast<Kind>(event.payload.front());
        const auto body = std::span<const std::byte>(event.payload).subspan(1);
        if (kind == Kind::created && state_ == SessionState::connecting
            && request_.front() == static_cast<std::byte>(Kind::create)) {
            code_ = room_wire::normalize_code(room_wire::read_text(body));
            if (!code_.empty()) state_ = SessionState::hosting;
        } else if (kind == Kind::paired && body.empty()
                   && (state_ == SessionState::hosting ||
                       (state_ == SessionState::connecting && request_.front() == static_cast<std::byte>(Kind::join)))) {
            state_ = SessionState::connected;
            output.push_back({Event::Kind::connected, {}});
        } else if (kind == Kind::rejected) {
            fail(room_wire::read_text(body));
        } else if (kind == Kind::data && state_ == SessionState::connected && !body.empty()) {
            output.push_back({Event::Kind::message, {body.begin(), body.end()}, event.channel});
        }
    }

    std::string relay_;
    std::uint16_t relay_port_;
    std::unique_ptr<Session> transport_;
    SessionState state_{SessionState::idle};
    std::string code_, error_;
    std::vector<std::byte> request_;
    Clock::time_point deadline_{};
};
}

std::unique_ptr<Session> make_room_session(std::string address, std::uint16_t port) {
    return std::make_unique<RoomSession>(std::move(address), port);
}

std::unique_ptr<Session> make_room_session(std::string endpoint) {
    const auto separator = endpoint.find(':');
    auto address = endpoint.substr(0, separator);
    constexpr std::string_view host_characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-";
    if (address.empty() || address.find_first_not_of(host_characters) != std::string::npos)
        throw std::invalid_argument("Relay endpoint must be a hostname or IPv4 address with an optional :port");

    unsigned port = room_relay_port;
    if (separator != std::string::npos) {
        const std::string_view suffix = std::string_view(endpoint).substr(separator + 1);
        const auto [end, error] = std::from_chars(suffix.data(), suffix.data() + suffix.size(), port);
        if (error != std::errc{} || end != suffix.data() + suffix.size() || port == 0 || port > 65535)
            throw std::invalid_argument("Relay endpoint port must be between 1 and 65535");
    }
    return make_room_session(std::move(address), static_cast<std::uint16_t>(port));
}

}
