// Purpose: Create the container class (World) for entities and their components, also holds template component storage

#pragma once
#include "engine/types.hpp"
#include <stdexcept>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <typeindex>
#include <any>
#include <memory>
#include <utility>

namespace engine
{
    struct StorageBase // Base storage template that every component inherits from
    {
        virtual ~StorageBase() = default;     // Cleanup function for when an object is destroyed
        virtual void remove(EntityId id) = 0; // Remove function that works for every type
        virtual std::size_t size() const = 0; // Check size function for the storage base
    };

    template <class T>
    struct Storage final : StorageBase // Template class that inherits from StorageBase
    {
        std::unordered_map<EntityId, T> column_;                     // Map that holds the actual component "column" of the spreadsheet
        void remove(EntityId id) override { column_.erase(id); }     // Removes entry for the id
        std::size_t size() const override { return column_.size(); } // Size function implementation
    };

    class World
    {
    public:
        EntityId create(Transform transform = {});
        bool destroy(EntityId id);
        Transform *find(EntityId id);                           // Returns a pointer to an entity's Transform so it can be read and changed
        const Transform *find(EntityId id) const;               // Same as above but for read only access
        std::size_t size() const { return transforms_.size(); } // Returns how many entities currently exist
        bool alive(EntityId id) const;

        template <class T>
        T &add(EntityId id, T value = {})
        {
            if (!alive(id))
                throw std::invalid_argument("Entity is not alive");
            auto &columnSlot = storages_[std::type_index(typeid(T))]; // Looks for component column, column_ptr is where in the spreadsheet you'd put a new column if it doesn't exist yet
            if (!columnSlot)
                columnSlot = std::make_unique<Storage<T>>();               // If spot is empty, create a template storage class for the component
            auto &cells = static_cast<Storage<T> &>(*columnSlot).column_;  // cells is the reference variable name for the specific column so that we can read and write entries
            auto [it, inserted] = items.try_emplace(id, std::move(value)); // Puts component value into the column if it doesn't exist yet and returns an iterator pointing at the entry and a bool for if insertion is successful
            if (!inserted)
                throw std::invalid_argument("Entity already has this component");
            return it->second; // Return reference to the stored component
        }

        template <class T>
        const T *get(EntityId id) const
        {
            if (!alive(id))
                return nullptr;
            const auto it = storages_.find(std::type_index(typeid(T))); // Looks for the component column and returns an iterator
            if (it == storages_.end())
                return nullptr;
            const auto &cells = static_cast<const Storage<T> &>(*it->second).column_; // Same type recovery as add
            const auto found = cells.find(id);
            return found == cells.end() ? nullptr : &found->second; // Returns address of the component as a pointer
        }

        template <class T>
        T *get(EntityId id) // Non-const version of get for changing the component through the returned pointer
        {
            return const_cast<T *>(std::as_const(*this).template get<T>(id)); // Calls the const get class and just removes the const
        }

        template <class T>
        bool remove(EntityId id)
        {
            const auto it = storages_.find(std::type_index(typeid(T))); // Same find column process as get
            if (it == storages_.end())
                return false;
            return static_cast<Storage<T> &>(*it->second).column_.erase(id) != 0; // Recovers type storage and returns a bool for if 1 or 0 columns were removed
        }

        template <class T>
        std::size_t count() const
        {
            auto it = storages_.find(std::type_index(typeid(T)));
            return it == storages_.end() ? 0 : it->second->size();
        }

    private:
        EntityId next_id_{1};
        std::map<EntityId, Transform> transforms_;

        std::unordered_map<std::type_index, std::unique_ptr<StorageBase>> storages_; // Map that holds all the different component storages
    };
}
