// Purpose: defines basic types that many subsystems use (entities, tranform component)

#pragma once
#include <cstdint>
namespace engine
{
    using EntityId = std::uint64_t; // Making a nicer name for 64-bit unsigned number IDs
    struct Vec2                     // 2D vector object for anything that takes 2 values
    {
        float x{0.0F};
        float y{0.0F};
    };
    inline constexpr EntityId invalid_entity = 0; // Like nullptr, states that 0 means no entity
    struct Transform                              // Transform component
    {
        Vec2 position{};
        float rotation{0.0F};
        Vec2 scale{1.0F, 1.0F};
    };
}
