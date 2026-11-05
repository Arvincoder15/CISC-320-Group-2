#include "engine/architecture.hpp"
#include <limits>
#include <stdexcept>
#include <map>
#include <algorithm>

namespace engine
{

    EntityId World::create(Transform transform)
    {
        if (next_id_ == std::numeric_limits<EntityId>::max())
            throw std::overflow_error("Entity ID space exhausted");
        const auto id = next_id_++;
        transforms_.emplace(id, transform);
        return id;
    }

    bool World::destroy(EntityId id)
    {
        if (transforms_.erase(id) != 0)
        {

            for (auto &pair : storages_)
            {
                pair.second->remove(id);
            };
            return true;
        };

        return false;
    }

    Transform *World::find(EntityId id)
    {
        const auto it = transforms_.find(id);
        return it == transforms_.end() ? nullptr : &it->second;
    }

    const Transform *World::find(EntityId id) const
    {
        const auto it = transforms_.find(id);
        return it == transforms_.end() ? nullptr : &it->second;
    }

    bool World::alive(EntityId id) const
    {
        return transforms_.find(id) != transforms_.end();
    }

}
