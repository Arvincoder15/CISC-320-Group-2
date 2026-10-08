#include "engine/interaction.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::InputBuffer input;
    CHECK(!input.consume().primary_pressed);
    input.press_primary();
    input.request_quit();
    const auto frame = input.consume();
    CHECK(frame.primary_pressed && frame.quit_requested);
    const auto next = input.consume();
    CHECK(!next.primary_pressed && !next.quit_requested);
}); }
