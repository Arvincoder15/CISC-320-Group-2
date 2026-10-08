#include "engine/interaction.hpp"
#include <cmath>
#include <stdexcept>
namespace engine {
void InputBuffer::set_move_x(float axis) {
    if (!std::isfinite(axis) || axis < -1 || axis > 1)
        throw std::invalid_argument("Movement axis must be finite and within [-1, 1]");
    pending_.move_x = axis;
}
InputFrame InputBuffer::consume() {
    const auto frame = pending_;
    pending_.primary_pressed = false;
    pending_.quit_requested = false;
    return frame;
}
}
