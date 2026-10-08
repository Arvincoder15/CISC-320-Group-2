#pragma once
namespace engine {
// Rohan: platform events map to actions; game logic never reads SDL key codes.
struct InputFrame { bool primary_pressed{false}; bool quit_requested{false}; };
class InputBuffer {
public:
    void press_primary() { pending_.primary_pressed = true; }
    void request_quit() { pending_.quit_requested = true; }
    InputFrame consume(); // One-shot actions, consumed once per simulation tick.
private:
    InputFrame pending_{};
};
}
