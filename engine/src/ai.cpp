#include "engine/ai.hpp"
#include <cmath>
#include <stdexcept>
namespace engine {
bool VerticalSteering::request_upward_action(const SteeringObservation& observation) const {
    if (!std::isfinite(observation.position_y) || !std::isfinite(observation.target_y)
        || !std::isfinite(observation.dead_zone) || observation.dead_zone < 0)
        throw std::invalid_argument("Invalid steering observation");
    return observation.position_y > observation.target_y + observation.dead_zone;
}
}
