#pragma once

namespace engine {

// One buffer per player. The platform adapter maps devices to action frames.
struct InputFrame {
    float move_x{0}; // Held axis in [-1, 1].
    bool primary_pressed{false}; // One-shot jump action.
    bool quit_requested{false};
};

class InputBuffer {
public:
    void set_move_x(float axis);
    void press_primary() { pending_.primary_pressed = true; }
    void request_quit() { pending_.quit_requested = true; }

    // Keeps held movement and clears one-shot actions.
    InputFrame consume();

private:
    InputFrame pending_{};
};

enum class PlayerAction {
    MoveLeft,
    MoveRight,
    Jump
};

// Tracks held actions for one player.
class PlayerInput {
public:
    void press(PlayerAction action);
    void release(PlayerAction action);

    // Clears held actions and pending input, such as on focus loss.
    void clear();

    InputFrame consume();

private:
    void update_movement();

    InputBuffer buffer_;
    bool left_held_{false};
    bool right_held_{false};
    bool jump_held_{false};
};

}