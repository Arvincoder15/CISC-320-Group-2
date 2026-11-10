#include "engine/physics.hpp"
#include <cmath>
#include <stdexcept>

namespace engine {
namespace {

// Penetration smaller than this is treated as touching. It absorbs float rounding
// after a body is snapped flush against a solid (pixel coordinates up to ~10^4).
constexpr float resolution_tolerance = 0.001F;

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

} // namespace engine
