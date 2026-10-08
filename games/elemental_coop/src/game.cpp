#include "elemental_coop/game.hpp"
#include <cmath>
#include <stdexcept>
namespace elemental_coop {
Game::Game(Settings settings) : settings_(settings) {
    if (!std::isfinite(settings.gravity) || settings.gravity <= 0
        || !std::isfinite(settings.move_speed) || settings.move_speed <= 0
        || !std::isfinite(settings.jump_speed) || settings.jump_speed <= 0
        || !std::isfinite(settings.floor_y))
        throw std::invalid_argument("Invalid game settings");
    for (auto& player : players_) player.entity = world_.create();
    restart();
}
void Game::restart() {
    state_ = RoundState::playing;
    exit_contact_updated_ = {};
    for (std::size_t i = 0; i < player_count; ++i) {
        const auto entity = players_[i].entity;
        players_[i] = PlayerState{};
        players_[i].entity = entity;
        players_[i].element = i == 0 ? Element::fire : Element::water;
        players_[i].body.acceleration.y = settings_.gravity;
        world_.find(entity)->position = {64.0F + 64.0F * static_cast<float>(i), settings_.floor_y};
    }
}
void Game::tick(const PlayerInputs& inputs, float dt) {
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Invalid tick duration");
    // Validate the whole frame before mutating either player.
    for (const auto& input : inputs) {
        if (!std::isfinite(input.move_x) || input.move_x < -1 || input.move_x > 1)
            throw std::invalid_argument("Invalid movement axis");
    }
    for (const auto& input : inputs) if (input.quit_requested) state_ = RoundState::quit;
    if (state_ != RoundState::playing || dt == 0) return;
    exit_contact_updated_ = {};
    for (std::size_t i = 0; i < player_count; ++i) {
        auto& player = players_[i];
        player.at_exit = false;
        player.body.velocity.x = inputs[i].move_x * settings_.move_speed;
        if (inputs[i].primary_pressed && player.grounded) {
            player.body.velocity.y = -settings_.jump_speed;
            player.grounded = false;
        }
        auto& transform = *world_.find(player.entity);
        engine::integrate(transform, player.body, dt);
        // Temporary flat-floor constraint, not a platform/tile collision solver.
        if (transform.position.y >= settings_.floor_y) {
            transform.position.y = settings_.floor_y;
            player.body.velocity.y = 0;
            player.grounded = true;
        }
    }
}
void Game::touch_hazard(std::size_t player_index, Hazard hazard) {
    auto& player = players_.at(player_index);
    if (state_ != RoundState::playing) return;
    const bool safe = (player.element == Element::fire && hazard == Hazard::lava)
        || (player.element == Element::water && hazard == Hazard::water);
    if (!safe) {
        player.alive = false;
        state_ = RoundState::lost;
    }
}
void Game::set_exit_contact(std::size_t player_index, bool touching) {
    auto& player = players_.at(player_index);
    if (state_ != RoundState::playing) return;
    player.at_exit = touching;
    exit_contact_updated_[player_index] = true;
    if (exit_contact_updated_[0] && exit_contact_updated_[1]
        && players_[0].alive && players_[1].alive && players_[0].at_exit && players_[1].at_exit)
        state_ = RoundState::won;
}
}
