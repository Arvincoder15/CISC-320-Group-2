// Purpose: reusable 2D movement and collision. Units follow ARCHITECTURE.md:
// pixels, +x right, +y down, seconds for dt, velocities in pixels/second.
// Engine physics knows nothing about elemental game rules.

#pragma once
#include "engine/types.hpp"
#include <cstdint>

namespace engine {

// ECS component for anything that moves. Initial values are supplied by the caller.
struct Body { Vec2 velocity{}; Vec2 acceleration{}; };

// Axis-aligned box: position is the top-left corner, size is width/height.
struct Aabb { Vec2 position{}; Vec2 size{}; };

// Semi-implicit Euler: velocity += acceleration * dt, then position += velocity * dt.
// dt must be finite and nonnegative.
void integrate(Transform& transform, Body& body, float dt);
bool overlaps(const Aabb& a, const Aabb& b); // Touching edges do not overlap.

// Collision layers are bit flags. Games name their own layers, e.g.
// `inline constexpr CollisionLayers layer_player = 1U << 1U;`.
using CollisionLayers = std::uint32_t;
inline constexpr CollisionLayers layer_none = 0;
inline constexpr CollisionLayers layer_default = 1U << 0U;
inline constexpr CollisionLayers layer_all = ~CollisionLayers{0};

// ECS component describing an entity's collision box relative to its Transform.
// `layer` is what this collider is; `mask` is which layers it reacts to.
// Rotation and scale of the Transform are ignored.
struct Collider {
    Vec2 offset{};
    Vec2 size{};
    CollisionLayers layer{layer_default};
    CollisionLayers mask{layer_all};
    bool trigger{false}; // Triggers report contacts but never block movement.
};

// World-space box for a collider attached to a transform.
Aabb bounds(const Transform& transform, const Collider& collider);
// Two colliders interact only when each one's mask accepts the other's layer.
bool layers_interact(const Collider& a, const Collider& b);

} // namespace engine
