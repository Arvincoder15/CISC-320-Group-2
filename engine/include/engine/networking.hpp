#pragma once
#include "engine/types.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>
namespace engine::net {
// Henry: one-peer connection used by the co-op game (ADR NET-4). Main thread only.
enum class SessionState { idle, hosting, connecting, connected, failed, disconnected };
enum class Channel : std::uint8_t { reliable = 0, unreliable = 1 };
struct Event {
    enum class Kind { connected, disconnected, message };
    Kind kind{Kind::message};
    std::vector<std::byte> payload; // only for message events
};
class Session {
public:
    virtual ~Session() = default;
    virtual bool host(std::uint16_t port) = 0;                        // start listening; false + last_error() on failure
    virtual bool join(const std::string& address, std::uint16_t port) = 0;
    virtual bool send(Channel channel, std::span<const std::byte> bytes) = 0; // false unless connected
    virtual std::vector<Event> poll() = 0;                              // non-blocking; call once per frame
    virtual SessionState state() const = 0;
    virtual const std::string& last_error() const = 0;
    virtual std::uint16_t port() const = 0;                             // port being hosted, 0 otherwise
    virtual void close() = 0;
};
// Real UDP connection via ENet (ADR NET-2). Port 0 in host() lets the OS pick; read it back with port().
std::unique_ptr<Session> make_enet_session();
// In-memory pair for tests. delay_polls: each event reaches the receiver on its (delay_polls + 1)-th poll.
// drop_percent applies to the unreliable channel only. Same seed -> same drops on every platform.
struct LinkConditions { int delay_polls{0}; int drop_percent{0}; unsigned seed{1}; };
std::pair<std::unique_ptr<Session>, std::unique_ptr<Session>> make_loopback_pair(LinkConditions conditions = {});

// Little-endian wire encoding (ADR NET-3). Never send raw structs.
class ByteWriter {
public:
    ByteWriter& u8(std::uint8_t value);
    ByteWriter& u32(std::uint32_t value);
    ByteWriter& f32(float value);
    const std::vector<std::byte>& bytes() const { return bytes_; }
private:
    std::vector<std::byte> bytes_;
};
// Reads return nullopt on short input; f32 also on NaN/infinity. Data from the network is untrusted.
class ByteReader {
public:
    explicit ByteReader(std::span<const std::byte> bytes) : bytes_(bytes) {}
    std::optional<std::uint8_t> u8();
    std::optional<std::uint32_t> u32();
    std::optional<float> f32();
    bool done() const { return offset_ == bytes_.size(); }
private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{0};
};
}
namespace engine {
struct StateSnapshot { std::uint64_t tick{0}; EntityId entity{invalid_entity}; Vec2 position{}; };
// Henry: replace the test transport with a real client/server adapter.
// This is an in-memory data contract, NOT a wire format. Never send raw structs.
class SnapshotTransport {
public:
    virtual ~SnapshotTransport() = default;
    virtual bool send(const StateSnapshot& snapshot) = 0;
    virtual std::optional<StateSnapshot> receive() = 0;
};
class LoopbackTransport final : public SnapshotTransport {
public:
    explicit LoopbackTransport(std::size_t capacity = 64) : capacity_(capacity) {}
    bool send(const StateSnapshot& snapshot) override;
    std::optional<StateSnapshot> receive() override;
private:
    std::size_t capacity_;
    std::deque<StateSnapshot> queue_;
};
}
