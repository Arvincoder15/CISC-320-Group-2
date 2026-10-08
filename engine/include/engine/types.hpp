#pragma once
#include <cstdint>
namespace engine {
// Screen coordinates: +x right, +y down. Positions in pixels; time in seconds.
struct Vec2 { float x{0.0F}; float y{0.0F}; };
using EntityId = std::uint64_t;
inline constexpr EntityId invalid_entity = 0;
struct Transform { Vec2 position{}; };
}
