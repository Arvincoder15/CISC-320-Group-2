#pragma once
#include "engine/types.hpp"
#include <map>
namespace engine {
// Sydney: minimal world storage, NOT a complete ECS. IDs are never reused.
class World {
public:
    EntityId create(Transform transform = {});
    bool destroy(EntityId id);
    Transform* find(EntityId id);
    const Transform* find(EntityId id) const;
    std::size_t size() const { return transforms_.size(); }
private:
    EntityId next_id_{1};
    std::map<EntityId, Transform> transforms_;
};
}
