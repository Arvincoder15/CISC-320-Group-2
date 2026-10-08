#pragma once
#include "engine/types.hpp"
namespace engine {
struct Body { Vec2 velocity{}; Vec2 acceleration{}; };
struct Aabb { Vec2 position{}; Vec2 size{}; };
// D'Artagnan: semi-implicit Euler starter; collision response/layers are TODO.
void integrate(Transform& transform, Body& body, float dt);
bool overlaps(const Aabb& a, const Aabb& b); // Touching edges do not overlap.
}
