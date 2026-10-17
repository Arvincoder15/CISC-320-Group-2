// Purpose: Create the entity container class (World)

#pragma once
#include "engine/types.hpp"
#include <map>
namespace engine
{
    // Sydney: minimal world storage, NOT a complete ECS. IDs are never reused.
    class World
    {
    public:
        EntityId create(Transform transform = {});
        bool destroy(EntityId id);
        Transform *find(EntityId id);                           // Returns a pointer to an entity's Transform so it can be read and changed
        const Transform *find(EntityId id) const;               // Same as above but for read only access
        std::size_t size() const { return transforms_.size(); } // Returns how many entities currently exist

        bool alive(EntityId id) const;

    private:
        EntityId next_id_{1};
        std::map<EntityId, Transform> transforms_;
    };
}
