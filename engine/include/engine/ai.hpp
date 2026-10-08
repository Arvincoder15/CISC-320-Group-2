#pragma once
namespace engine {
// Rocco: reusable two-state patrol controller for an optional moving hazard/NPC.
// This does not control either human player or implement pathfinding.
class PatrolController {
public:
    float direction(float position_x, float left_bound, float right_bound);
private:
    float direction_{1};
};
}
