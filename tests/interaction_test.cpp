#include "engine/interaction.hpp"
#include "test_support.hpp"
#include <limits>
int main() { return run_test([] {
    engine::InputBuffer first, second;
    first.set_move_x(1);
    second.set_move_x(-1);
    first.press_primary();
    first.request_quit();
    const auto frame = first.consume();
    CHECK(frame.move_x == 1 && frame.primary_pressed && frame.quit_requested);
    const auto next = first.consume();
    CHECK(next.move_x == 1 && !next.primary_pressed && !next.quit_requested);
    CHECK(second.consume().move_x == -1);
    first.set_move_x(0);
    CHECK(first.consume().move_x == 0);
    CHECK(throws_as<std::invalid_argument>([&] { first.set_move_x(2); }));
    CHECK(throws_as<std::invalid_argument>([&] { first.set_move_x(std::numeric_limits<float>::quiet_NaN()); }));
}); }
