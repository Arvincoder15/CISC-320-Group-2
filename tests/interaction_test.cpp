#include "engine/interaction.hpp"
#include "test_support.hpp"

#include <limits>
#include <stdexcept>

int main() {
    return run_test([] {
        // Existing InputBuffer behaviour.
        engine::InputBuffer first, second;

        first.set_move_x(1);
        second.set_move_x(-1);
        first.press_primary();
        first.request_quit();

        const auto frame = first.consume();
        CHECK(frame.move_x == 1);
        CHECK(frame.primary_pressed);
        CHECK(frame.quit_requested);

        const auto next = first.consume();
        CHECK(next.move_x == 1);
        CHECK(!next.primary_pressed);
        CHECK(!next.quit_requested);
        CHECK(second.consume().move_x == -1);

        first.set_move_x(0);
        CHECK(first.consume().move_x == 0);

        CHECK(throws_as<std::invalid_argument>([&] {
            first.set_move_x(2);
        }));

        CHECK(throws_as<std::invalid_argument>([&] {
            first.set_move_x(
                std::numeric_limits<float>::quiet_NaN());
        }));

        using engine::PlayerAction;

        engine::PlayerInput player_one;
        engine::PlayerInput player_two;

        // Players have independent movement and jump input.
        player_one.press(PlayerAction::MoveLeft);
        player_two.press(PlayerAction::MoveRight);
        player_one.press(PlayerAction::Jump);

        const auto one = player_one.consume();
        const auto two = player_two.consume();

        CHECK(one.move_x == -1);
        CHECK(one.primary_pressed);
        CHECK(two.move_x == 1);
        CHECK(!two.primary_pressed);

        // Movement persists, but jump is consumed once.
        const auto held = player_one.consume();
        CHECK(held.move_x == -1);
        CHECK(!held.primary_pressed);

        // Repeated key-down events do not repeat jump.
        player_one.press(PlayerAction::Jump);
        CHECK(!player_one.consume().primary_pressed);

        // Releasing and pressing jump allows another action.
        player_one.release(PlayerAction::Jump);
        player_one.press(PlayerAction::Jump);
        CHECK(player_one.consume().primary_pressed);

        // Opposite directions cancel while both are held.
        player_one.press(PlayerAction::MoveRight);
        CHECK(player_one.consume().move_x == 0);

        // Releasing left preserves held right movement.
        player_one.release(PlayerAction::MoveLeft);
        CHECK(player_one.consume().move_x == 1);

        // Releasing both directions stops movement.
        player_one.release(PlayerAction::MoveRight);
        CHECK(player_one.consume().move_x == 0);

        // Releasing right preserves held left movement.
        player_one.press(PlayerAction::MoveLeft);
        player_one.press(PlayerAction::MoveRight);
        player_one.release(PlayerAction::MoveRight);
        CHECK(player_one.consume().move_x == -1);

        player_one.release(PlayerAction::MoveLeft);

        // A quick press and release still delivers one jump.
        player_one.release(PlayerAction::Jump);
        player_one.press(PlayerAction::Jump);
        player_one.release(PlayerAction::Jump);
        CHECK(player_one.consume().primary_pressed);
        CHECK(!player_one.consume().primary_pressed);

        // Clearing removes held movement and pending jump.
        player_one.press(PlayerAction::Jump);
        player_one.press(PlayerAction::MoveLeft);
        player_one.clear();

        const auto cleared = player_one.consume();
        CHECK(cleared.move_x == 0);
        CHECK(!cleared.primary_pressed);
        CHECK(!cleared.quit_requested);

        // Clearing one player does not affect the other.
        CHECK(player_two.consume().move_x == 1);

        // Input works again after clearing.
        player_one.press(PlayerAction::MoveRight);
        player_one.press(PlayerAction::Jump);

        const auto resumed = player_one.consume();
        CHECK(resumed.move_x == 1);
        CHECK(resumed.primary_pressed);
    });
}