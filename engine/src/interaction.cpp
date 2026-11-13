#include "engine/interaction.hpp"

#include <cmath>
#include <stdexcept>

namespace engine {

void InputBuffer::set_move_x(float axis) {
    if (!std::isfinite(axis) || axis < -1 || axis > 1) {
        throw std::invalid_argument(
            "Movement axis must be finite and within [-1, 1]");
    }

    pending_.move_x = axis;
}

InputFrame InputBuffer::consume() {
    const auto frame = pending_;
    pending_.primary_pressed = false;
    pending_.quit_requested = false;
    return frame;
}

void PlayerInput::update_movement() {
    const float axis =
        static_cast<float>(right_held_) -
        static_cast<float>(left_held_);

    buffer_.set_move_x(axis);
}

void PlayerInput::press(PlayerAction action) {
    switch (action) {
    case PlayerAction::MoveLeft:
        left_held_ = true;
        update_movement();
        break;

    case PlayerAction::MoveRight:
        right_held_ = true;
        update_movement();
        break;

    case PlayerAction::Jump:
        if (!jump_held_) {
            buffer_.press_primary();
        }

        jump_held_ = true;
        break;
    }
}

void PlayerInput::release(PlayerAction action) {
    switch (action) {
    case PlayerAction::MoveLeft:
        left_held_ = false;
        update_movement();
        break;

    case PlayerAction::MoveRight:
        right_held_ = false;
        update_movement();
        break;

    case PlayerAction::Jump:
        jump_held_ = false;
        break;
    }
}

void PlayerInput::clear() {
    left_held_ = false;
    right_held_ = false;
    jump_held_ = false;
    buffer_ = InputBuffer{};
}

InputFrame PlayerInput::consume() {
    return buffer_.consume();
}

}