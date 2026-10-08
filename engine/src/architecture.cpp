#include "engine/architecture.hpp"
#include <limits>
#include <stdexcept>
namespace engine {
EntityId World::create(Transform transform) {
    if (next_id_ == std::numeric_limits<EntityId>::max())
        throw std::overflow_error("Entity ID space exhausted");
    const auto id = next_id_++;
    transforms_.emplace(id, transform);
    return id;
}
bool World::destroy(EntityId id) { return transforms_.erase(id) != 0; }
Transform* World::find(EntityId id) {
    const auto it = transforms_.find(id);
    return it == transforms_.end() ? nullptr : &it->second;
}
const Transform* World::find(EntityId id) const {
    const auto it = transforms_.find(id);
    return it == transforms_.end() ? nullptr : &it->second;
}
}
