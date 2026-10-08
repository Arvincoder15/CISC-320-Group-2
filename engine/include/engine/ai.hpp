#pragma once
namespace engine {
// Rocco: reusable target-following decision; the game maps it to an action.
// Positive y points down. Neither bird rules nor world ownership live here.
struct SteeringObservation { float position_y{0}; float target_y{0}; float dead_zone{0}; };
class VerticalSteering {
public:
    bool request_upward_action(const SteeringObservation& observation) const;
};
}
