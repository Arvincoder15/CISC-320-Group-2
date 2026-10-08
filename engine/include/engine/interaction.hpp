#pragma once
namespace engine {
// One buffer per player. The platform adapter maps devices to action frames.
struct InputFrame {
    float move_x{0}; // Held axis in [-1, 1].
    bool primary_pressed{false}; // One-shot; the co-op game maps this to jump.
    bool quit_requested{false};
};
class InputBuffer {
public:
    void set_move_x(float axis);
    void press_primary() { pending_.primary_pressed = true; }
    void request_quit() { pending_.quit_requested = true; }
    InputFrame consume(); // Keeps the held axis, clears one-shot actions.
private:
    InputFrame pending_{};
};
}
