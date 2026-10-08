#include "engine/physics.hpp"
#include <cmath>
#include <stdexcept>
namespace engine {
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
}
