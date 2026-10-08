#include "engine/networking.hpp"
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
