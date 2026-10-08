#pragma once
#include "engine/architecture.hpp"
#include "engine/interaction.hpp"
#include "engine/physics.hpp"
namespace flappy {
// Blake: illustrative values only; agree tuning and multiplayer rules as a team.
struct Settings { float gravity{900}; float flap_speed{300}; };
class Game {
public:
    explicit Game(Settings settings = {});
    void tick(const engine::InputFrame& input, float dt);
    engine::EntityId player() const { return player_; }
    const engine::World& world() const { return world_; }
    bool running() const { return running_; }
private:
    Settings settings_;
    engine::World world_;
    engine::EntityId player_;
    engine::Body body_{};
    bool running_{true};
};
}
