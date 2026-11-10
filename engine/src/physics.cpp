#include "engine/physics.hpp"
#include "engine/architecture.hpp"
#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace engine {
namespace {

// Penetration smaller than this is treated as touching. It absorbs float rounding
// after a body is snapped flush against a solid (pixel coordinates up to ~10^4).
constexpr float resolution_tolerance = 0.001F;

bool finite(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }

// Like overlaps(), but penetration up to resolution_tolerance counts as touching.
bool penetrates(const Aabb& a, const Aabb& b) {
    return a.position.x < b.position.x + b.size.x - resolution_tolerance
        && a.position.x + a.size.x > b.position.x + resolution_tolerance
        && a.position.y < b.position.y + b.size.y - resolution_tolerance
        && a.position.y + a.size.y > b.position.y + resolution_tolerance;
}

struct Solid {
    EntityId id;
    Aabb box;
};

// Moves along x, then pushes the body flush against any solid it entered.
void move_x(Transform& transform, Body& body, const Collider& collider, ContactState& contacts,
            const std::vector<Solid>& solids, float dt) {
    const float dx = body.velocity.x * dt;
    if (dx == 0) return;
    transform.position.x += dx;
    for (const auto& solid : solids) {
        if (!penetrates(bounds(transform, collider), solid.box)) continue;
        if (dx > 0) {
            transform.position.x = solid.box.position.x - collider.size.x - collider.offset.x;
            contacts.wall_right = true;
        } else {
            transform.position.x = solid.box.position.x + solid.box.size.x - collider.offset.x;
            contacts.wall_left = true;
        }
        body.velocity.x = 0;
    }
}

// Moves along y, landing on floors (moving down) or bumping ceilings (moving up).
void move_y(Transform& transform, Body& body, const Collider& collider, ContactState& contacts,
            const std::vector<Solid>& solids, float dt) {
    const float dy = body.velocity.y * dt;
    if (dy == 0) return;
    transform.position.y += dy;
    for (const auto& solid : solids) {
        if (!penetrates(bounds(transform, collider), solid.box)) continue;
        if (dy > 0) {
            transform.position.y = solid.box.position.y - collider.size.y - collider.offset.y;
            contacts.grounded = true;
        } else {
            transform.position.y = solid.box.position.y + solid.box.size.y - collider.offset.y;
            contacts.ceiling = true;
        }
        body.velocity.y = 0;
    }
}

// Resting contact: a body standing still (or with zero gravity) never moves into
// the floor, so look a thin skin below it instead. Rising bodies are never grounded.
// A box that only shares a vertical edge or a corner with a solid is not supported.
void probe_ground(const Transform& transform, const Body& body, const Collider& collider,
                  ContactState& contacts, const std::vector<Solid>& solids, float skin) {
    if (body.velocity.y < 0) return;
    auto probe = bounds(transform, collider);
    probe.position.y += skin;
    for (const auto& solid : solids) {
        if (!penetrates(probe, solid.box)) continue;
        contacts.grounded = true;
        contacts.ground = solid.id; // Lowest entity ID wins when several support the body.
        return;
    }
}

} // namespace

void integrate(Transform& transform, Body& body, float dt) {
    if (!std::isfinite(dt) || dt < 0.0F) throw std::invalid_argument("dt must be finite and nonnegative");
    body.velocity.x += body.acceleration.x * dt;
    body.velocity.y += body.acceleration.y * dt;
    transform.position.x += body.velocity.x * dt;
    transform.position.y += body.velocity.y * dt;
}

bool overlaps(const Aabb& a, const Aabb& b) {
    if (a.size.x <= 0 || a.size.y <= 0 || b.size.x <= 0 || b.size.y <= 0) return false;
    return a.position.x < b.position.x + b.size.x && a.position.x + a.size.x > b.position.x
        && a.position.y < b.position.y + b.size.y && a.position.y + a.size.y > b.position.y;
}

Aabb bounds(const Transform& transform, const Collider& collider) {
    return {{transform.position.x + collider.offset.x, transform.position.y + collider.offset.y}, collider.size};
}

bool layers_interact(const Collider& a, const Collider& b) {
    return (a.layer & b.mask) != 0 && (b.layer & a.mask) != 0;
}

void validate(const PhysicsSettings& settings) {
    if (!std::isfinite(settings.gravity.x) || !std::isfinite(settings.gravity.y))
        throw std::invalid_argument("Physics gravity must be finite");
    // Written as !(x > 0) so NaN is rejected; infinity is allowed (no limit).
    if (!(settings.max_speed.x > 0) || !(settings.max_speed.y > 0))
        throw std::invalid_argument("Physics speed limits must be positive");
    if (!std::isfinite(settings.max_substep) || settings.max_substep <= 0)
        throw std::invalid_argument("Physics max_substep must be finite and positive");
    if (settings.max_substeps < 1)
        throw std::invalid_argument("Physics max_substeps must be at least 1");
    if (!std::isfinite(settings.contact_skin) || settings.contact_skin <= 2 * resolution_tolerance)
        throw std::invalid_argument("Physics contact_skin must be finite and thicker than the resolution tolerance");
}

PhysicsSystem::PhysicsSystem(PhysicsSettings settings) : settings_(settings) {
    validate(settings_);
}

void PhysicsSystem::add(const World& world, EntityId id) {
    if (!world.alive(id)) throw std::invalid_argument("Cannot add a dead entity to physics");
    tracked_.insert(id);
}

bool PhysicsSystem::remove(EntityId id) {
    return tracked_.erase(id) != 0;
}

void PhysicsSystem::prune(const World& world) {
    std::erase_if(tracked_, [&](EntityId id) { return !world.alive(id); });
}

void PhysicsSystem::validate_components(const World& world) const {
    for (const auto id : tracked_) {
        if (!finite(world.find(id)->position))
            throw std::invalid_argument("Physics entity has a non-finite position");
        if (const auto* body = world.get<Body>(id); body && (!finite(body->velocity) || !finite(body->acceleration)))
            throw std::invalid_argument("Physics body has non-finite velocity or acceleration");
        if (const auto* collider = world.get<Collider>(id); collider
            && (!finite(collider->offset) || !std::isfinite(collider->size.x) || !std::isfinite(collider->size.y)
                || collider->size.x <= 0 || collider->size.y <= 0))
            throw std::invalid_argument("Physics collider needs a finite offset and a finite positive size");
    }
}

void PhysicsSystem::accelerate(Body& body, float dt) const {
    body.velocity.x += (body.acceleration.x + settings_.gravity.x) * dt;
    body.velocity.y += (body.acceleration.y + settings_.gravity.y) * dt;
    body.velocity.x = std::clamp(body.velocity.x, -settings_.max_speed.x, settings_.max_speed.x);
    body.velocity.y = std::clamp(body.velocity.y, -settings_.max_speed.y, settings_.max_speed.y);
}

void PhysicsSystem::step(World& world, float dt) {
    if (!std::isfinite(dt) || dt < 0.0F) throw std::invalid_argument("dt must be finite and nonnegative");
    prune(world);
    // Validate everything first so a bad component never leaves a half-stepped world.
    validate_components(world);
    if (dt == 0) return;

    // Static solids do not move during a step, so their boxes are computed once.
    std::vector<Solid> solids;
    std::vector<EntityId> colliding_bodies;
    for (const auto id : tracked_) {
        const auto* collider = world.get<Collider>(id);
        if (collider == nullptr || collider->trigger) continue;
        if (world.get<Body>(id) == nullptr) solids.push_back({id, bounds(*world.find(id), *collider)});
        else colliding_bodies.push_back(id);
    }
    // Add every ContactState before taking component pointers below.
    for (const auto id : colliding_bodies) {
        if (auto* contacts = world.get<ContactState>(id)) *contacts = {};
        else world.add<ContactState>(id);
    }

    for (const auto id : tracked_) {
        auto* body = world.get<Body>(id);
        if (body == nullptr) continue;
        auto& transform = *world.find(id);
        accelerate(*body, dt);
        const auto* collider = world.get<Collider>(id);
        if (collider == nullptr || collider->trigger) {
            // Unresolved movers: plain bodies and moving triggers.
            transform.position.x += body->velocity.x * dt;
            transform.position.y += body->velocity.y * dt;
            continue;
        }
        auto& contacts = *world.get<ContactState>(id);
        // Axis-separated resolution: x first, so walking along a floor made of
        // several adjacent tiles never catches on the seams between them.
        move_x(transform, *body, *collider, contacts, solids, dt);
        move_y(transform, *body, *collider, contacts, solids, dt);
        probe_ground(transform, *body, *collider, contacts, solids, settings_.contact_skin);
    }
}

} // namespace engine
