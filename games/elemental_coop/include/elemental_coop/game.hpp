#pragma once
#include "engine/architecture.hpp"
#include "engine/interaction.hpp"
#include "engine/physics.hpp"
#include <array>
namespace elemental_coop {
inline constexpr std::size_t player_count = 2;
enum class Element { fire, water };
enum class Hazard { lava, water, toxic };
enum class RoundState { playing, lost, won, quit };
// Illustrative tuning, pending team review. Positions represent character feet.
struct Settings { float gravity{900}; float move_speed{180}; float jump_speed{360}; float floor_y{320}; };
struct PlayerState {
    engine::EntityId entity{engine::invalid_entity};
    Element element{Element::fire};
    engine::Body body{};
    bool grounded{true};
    bool alive{true};
    bool at_exit{false};
};
using PlayerInputs = std::array<engine::InputFrame, player_count>;
class Game {
public:
    explicit Game(Settings settings = {});
    void tick(const PlayerInputs& inputs, float dt);
    void restart();
    // Integration seams: a future collision adapter supplies contacts after tick.
    void touch_hazard(std::size_t player_index, Hazard hazard);
    // Pass true only for this character's matching exit; refresh both every tick.
    void set_exit_contact(std::size_t player_index, bool touching);
    const PlayerState& player(std::size_t index) const { return players_.at(index); }
    const engine::World& world() const { return world_; }
    RoundState state() const { return state_; }
private:
    Settings settings_;
    engine::World world_;
    std::array<PlayerState, player_count> players_{};
    RoundState state_{RoundState::playing};
    std::array<bool, player_count> exit_contact_updated_{};
};
}
