// Purpose: reusable 2D movement and collision. Units follow ARCHITECTURE.md:
// pixels, +x right, +y down, seconds for dt, velocities in pixels/second.
// Engine physics knows nothing about elemental game rules.

#pragma once
#include "engine/types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>

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

// Movement policy shared by every body a PhysicsSystem steps.
// - gravity (pixels/s^2) is added to each Body's own acceleration, so a game that
//   already stores gravity in Body::acceleration should leave this at zero.
// - max_speed clamps |velocity.x| and |velocity.y| after acceleration (terminal
//   velocity). Infinity means unlimited.
// - Steps longer than max_substep are split into equal substeps (at most
//   max_substeps) to reduce tunnelling. The caller owns the fixed-step loop; the
//   proposed default is one 1/60 s tick per call.
// - contact_skin is how far below a body the grounded probe looks (pixels).
struct PhysicsSettings {
    Vec2 gravity{};
    Vec2 max_speed{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};
    float max_substep{1.0F / 60.0F};
    int max_substeps{8};
    float contact_skin{0.01F};
};

// Throws std::invalid_argument for non-finite gravity, nonpositive or NaN speed
// limits, nonpositive/non-finite max_substep, max_substeps < 1, or a contact skin
// that is non-finite or too thin to detect resting contact.
void validate(const PhysicsSettings& settings);

class World;

// Steps the entities registered with it. The World owns all component data; the
// system only remembers which entity IDs to simulate. Components used:
// - Transform (every entity): position.
// - Body: the entity moves. Entities without a Body never move.
// Destroyed entities are dropped automatically at the start of the next step.
// Single-threaded; the World must outlive each call that receives it.
class PhysicsSystem {
public:
    explicit PhysicsSystem(PhysicsSettings settings = {}); // Throws for invalid settings.
    const PhysicsSettings& settings() const { return settings_; }

    // Registers a live entity (throws std::invalid_argument otherwise). Adding an
    // entity twice is harmless. Components may be added before or after.
    void add(const World& world, EntityId id);
    bool remove(EntityId id); // False if the entity was not registered.
    bool contains(EntityId id) const { return tracked_.count(id) != 0; }
    std::size_t size() const { return tracked_.size(); }

    // Advances every registered body by dt seconds (finite and nonnegative).
    // dt == 0 changes nothing. Bodies are updated in ascending entity-ID order.
    // Throws std::invalid_argument, before changing any component, for a bad dt,
    // a non-finite position/velocity/acceleration, or a collider whose size is not
    // finite and positive or whose offset is not finite.
    void step(World& world, float dt);

private:
    void prune(const World& world);
    void validate_components(const World& world) const;
    void accelerate(Body& body, float dt) const;

    PhysicsSettings settings_;
    std::set<EntityId> tracked_;
};

} // namespace engine
