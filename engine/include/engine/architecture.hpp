// Purpose: Create the container class (World) for entities and their components

#pragma once
#include "engine/types.hpp"
#include <map>
#include <unordered_map>

namespace engine
{
    struct TempHealth
    { // Temporary for component building
        int hp{0};
    };

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

        TempHealth &add_health(EntityId id, TempHealth value);
        TempHealth *get_health(EntityId id);
        const TempHealth *get_health(EntityId id) const;
        bool remove_health(EntityId id);

    private:
        EntityId next_id_{1};
        std::map<EntityId, Transform> transforms_;
        std::unordered_map<EntityId, TempHealth> health_;
    };

}
