#include "flappy/game.hpp"
#include <cmath>
#include <stdexcept>
namespace flappy {
Game::Game(Settings settings) : settings_(settings), player_(world_.create()) {
    if (!std::isfinite(settings.gravity) || settings.gravity < 0
        || !std::isfinite(settings.flap_speed) || settings.flap_speed <= 0)
        throw std::invalid_argument("Invalid game settings");
    body_.acceleration.y = settings_.gravity;
}
void Game::tick(const engine::InputFrame& input, float dt) {
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Invalid tick duration");
    if (input.quit_requested) running_ = false;
    if (!running_) return;
    if (input.primary_pressed) body_.velocity.y = -settings_.flap_speed;
    engine::integrate(*world_.find(player_), body_, dt);
}
}
