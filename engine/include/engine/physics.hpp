// Purpose: reusable 2D movement and collision. Units follow ARCHITECTURE.md:
// pixels, +x right, +y down, seconds for dt, velocities in pixels/second.
// Engine physics knows nothing about elemental game rules.

#pragma once
#include "engine/types.hpp"
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

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

// ECS component written by PhysicsSystem::step for every body with a solid
// collider (added automatically if missing). Flags describe the most recent
// positive-duration step only and are cleared at the start of each step.
struct ContactState {
    bool grounded{false};   // Landed on, or resting within contact_skin of, a solid below.
    bool ceiling{false};    // Bumped a solid above this step (upward velocity zeroed).
    bool wall_left{false};  // Pushed into a solid on the left this step.
    bool wall_right{false}; // Pushed into a solid on the right this step.
    // The solid currently supporting the body (lowest ID if several), or
    // invalid_entity. It may be destroyed later; look it up through the World.
    EntityId ground{invalid_entity};
};

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

// Trigger contact transitions between one dynamic body and one trigger.
// Engine physics only reports overlaps; the game decides what a hazard, exit or
// pressure plate does with them.
enum class ContactPhase { enter, stay, leave };
struct CollisionEvent {
    ContactPhase phase{ContactPhase::enter};
    EntityId body{invalid_entity};    // Entity with Body + solid Collider.
    EntityId trigger{invalid_entity}; // Entity with a trigger Collider.
    bool operator==(const CollisionEvent&) const = default;
};

class World;

// Steps the entities registered with it. The World owns all component data; the
// system only remembers which entity IDs to simulate. Components used:
// - Transform (every entity): position.
// - Body: the entity moves. Entities without a Body never move.
// - Collider without Body, trigger == false: a static solid (floor, wall, platform).
// - Body + solid Collider: a dynamic body, stopped by static solids whose layers
//   interact with it. Dynamic bodies do not push or block each other.
// - Collider with trigger == true: never blocks; reported through events().
//   A trigger with a Body (e.g. a patrolling hazard) moves without collision.
// - ContactState: output for dynamic bodies.
// Destroyed entities are dropped automatically at the start of the next step.
// Single-threaded; the World must outlive each call that receives it.
//
// Known limitations (fast-moving collision and scope):
// - Discrete overlap tests, not swept collision. A body moving further in one
//   substep than its own size plus a solid's thickness can tunnel through it.
//   Substepping, speed limits and thicker level geometry keep this in check;
//   e.g. with 1/60 s substeps a 32 px tall body cannot fall through any floor
//   below 1920 px/s (32 px per substep). A step longer than max_substep * max_substeps uses
//   larger substeps, so tunnelling becomes possible again.
// - A body that starts a step already overlapping a solid is not pushed out.
// - Static solids are assumed not to move; moving platforms are not supported.
// - Broad phase is brute force (bodies x solids per substep); fine for small rooms.
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
    // dt is split into ceil(dt / max_substep) equal substeps, capped at
    // max_substeps; each substep integrates and resolves every body once.
    // Throws std::invalid_argument, before changing any component, for a bad dt,
    // a non-finite position/velocity/acceleration, or a collider whose size is not
    // finite and positive or whose offset is not finite.
    void step(World& world, float dt);

    // Trigger events produced by the latest step, ordered by (body, trigger) ID.
    // enter: overlap began; stay: overlap continues; leave: overlap ended, or one
    // entity was destroyed, removed from physics, or stopped being a body/trigger
    // (IDs in leave events may no longer be alive). Layers must interact.
    // A dt == 0 step clears events but keeps the tracked overlaps. The reference
    // is borrowed and is invalidated by the next step() or reset().
    const std::vector<CollisionEvent>& events() const { return events_; }

    // Forgets all tracked overlaps and pending events without emitting leave
    // events. Call after a game restart has repositioned its entities: bodies
    // that start inside a trigger then receive a fresh enter event.
    void reset();

    // Registered, live entities whose collider (solid or trigger) overlaps `area`
    // and whose layer is in `mask`, in ascending ID order. Touching edges do not
    // count. Intended for AI sight/space checks and gameplay placement tests.
    std::vector<EntityId> query(const World& world, const Aabb& area, CollisionLayers mask = layer_all) const;

private:
    void prune(const World& world);
    void validate_components(const World& world) const;
    void accelerate(Body& body, float dt) const;

    using TriggerPair = std::pair<EntityId, EntityId>; // (body, trigger)
    std::set<TriggerPair> find_trigger_overlaps(const World& world) const;
    void emit_events(const std::set<TriggerPair>& current);

    PhysicsSettings settings_;
    std::set<EntityId> tracked_;
    std::set<TriggerPair> overlapping_;
    std::vector<CollisionEvent> events_;
};

} // namespace engine
