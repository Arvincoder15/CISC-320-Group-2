#include "engine/networking.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <random>
namespace engine {
bool LoopbackTransport::send(const StateSnapshot& snapshot) {
    if (queue_.size() >= capacity_) return false;
    queue_.push_back(snapshot);
    return true;
}
std::optional<StateSnapshot> LoopbackTransport::receive() {
    if (queue_.empty()) return std::nullopt;
    const auto snapshot = queue_.front();
    queue_.pop_front();
    return snapshot;
}
}

namespace engine::net {
ByteWriter& ByteWriter::u8(std::uint8_t value) {
    bytes_.push_back(static_cast<std::byte>(value));
    return *this;
}
ByteWriter& ByteWriter::u32(std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) bytes_.push_back(static_cast<std::byte>((value >> shift) & 0xFFu));
    return *this;
}
ByteWriter& ByteWriter::f32(float value) { return u32(std::bit_cast<std::uint32_t>(value)); }

std::optional<std::uint8_t> ByteReader::u8() {
    if (offset_ >= bytes_.size()) return std::nullopt;
    return std::to_integer<std::uint8_t>(bytes_[offset_++]);
}
std::optional<std::uint32_t> ByteReader::u32() {
    if (bytes_.size() - offset_ < 4) return std::nullopt;
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) value |= std::to_integer<std::uint32_t>(bytes_[offset_++]) << shift;
    return value;
}
std::optional<float> ByteReader::f32() {
    const auto bits = u32();
    if (!bits) return std::nullopt;
    const auto value = std::bit_cast<float>(*bits);
    if (!std::isfinite(value)) return std::nullopt;
    return value;
}

namespace {
struct Pending { int due; Event event; };
// Shared by both ends of a loopback pair; index 0 and 1 are the two sessions.
struct Link {
    LinkConditions conditions;
    std::mt19937 rng;
    std::array<std::deque<Pending>, 2> inbox;
    std::array<int, 2> polls{};
    std::array<SessionState, 2> state{SessionState::idle, SessionState::idle};
    std::array<bool, 2> listening{}; // like ENet: a host accepts a new joiner after one leaves, until close()
    void deliver(int to, Event event) { inbox[to].push_back({polls[to] + conditions.delay_polls, std::move(event)}); }
};

class LoopbackSession final : public Session {
public:
    LoopbackSession(std::shared_ptr<Link> link, int side) : link_(std::move(link)), me_(side), peer_(1 - side) {}
    ~LoopbackSession() override { close(); }
    bool host(std::uint16_t port) override {
        if (busy()) return fail("Session already in use");
        link_->state[me_] = SessionState::hosting;
        link_->listening[me_] = true;
        port_ = port;
        return true;
    }
    bool join(const std::string&, std::uint16_t) override {
        if (busy()) return fail("Session already in use");
        if (!link_->listening[peer_]) {
            link_->state[me_] = SessionState::failed;
            return fail("Could not reach host");
        }
        link_->state[me_] = SessionState::connecting;
        link_->deliver(peer_, {Event::Kind::connected, {}});
        link_->deliver(me_, {Event::Kind::connected, {}});
        return true;
    }
    bool send(Channel channel, std::span<const std::byte> bytes) override {
        if (link_->state[me_] != SessionState::connected) return false;
        const auto drop = static_cast<unsigned>(link_->conditions.drop_percent);
        if (channel == Channel::unreliable && link_->rng() % 100 < drop) return true; // lost in transit
        link_->deliver(peer_, {Event::Kind::message, {bytes.begin(), bytes.end()}, channel});
        return true;
    }
    std::vector<Event> poll() override {
        auto& inbox = link_->inbox[me_];
        if (link_->state[me_] == SessionState::idle) {
            inbox.clear(); // closed sessions ignore late traffic
            return {};
        }
        const int now = ++link_->polls[me_];
        std::vector<Event> events;
        while (!inbox.empty() && inbox.front().due < now) {
            auto event = std::move(inbox.front().event);
            inbox.pop_front();
            if (event.kind == Event::Kind::connected) link_->state[me_] = SessionState::connected;
            if (event.kind == Event::Kind::disconnected) link_->state[me_] = SessionState::disconnected;
            events.push_back(std::move(event));
        }
        return events;
    }
    SessionState state() const override { return link_->state[me_]; }
    const std::string& last_error() const override { return error_; }
    std::uint16_t port() const override { return port_; }
    void close() override {
        const auto peer = link_->state[peer_];
        if (peer == SessionState::connecting || peer == SessionState::connected)
            link_->deliver(peer_, {Event::Kind::disconnected, {}});
        link_->state[me_] = SessionState::idle;
        link_->listening[me_] = false;
        link_->inbox[me_].clear();
        port_ = 0;
    }
private:
    bool busy() const {
        const auto s = link_->state[me_];
        return s == SessionState::hosting || s == SessionState::connecting || s == SessionState::connected;
    }
    bool fail(std::string message) {
        error_ = std::move(message);
        return false;
    }
    std::shared_ptr<Link> link_;
    int me_;
    int peer_;
    std::uint16_t port_{0};
    std::string error_;
};
}

std::pair<std::unique_ptr<Session>, std::unique_ptr<Session>> make_loopback_pair(LinkConditions conditions) {
    auto link = std::make_shared<Link>();
    link->conditions = conditions;
    link->rng.seed(conditions.seed);
    return {std::make_unique<LoopbackSession>(link, 0), std::make_unique<LoopbackSession>(link, 1)};
}
}
