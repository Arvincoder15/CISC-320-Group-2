#include "engine/ai.hpp"
#include <cmath>
#include <stdexcept>
namespace engine {
float PatrolController::direction(float position_x, float left_bound, float right_bound) {
    if (!std::isfinite(position_x) || !std::isfinite(left_bound) || !std::isfinite(right_bound)
        || left_bound >= right_bound)
        throw std::invalid_argument("Patrol requires a finite position and ordered bounds");
    if (position_x >= right_bound) direction_ = -1;
    else if (position_x <= left_bound) direction_ = 1;
    return direction_;
}
}
