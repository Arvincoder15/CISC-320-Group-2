#pragma once
#include "engine/types.hpp"
#include <deque>
#include <optional>
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
