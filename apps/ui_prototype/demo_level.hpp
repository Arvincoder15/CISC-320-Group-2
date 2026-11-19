#pragma once

#include "elemental_coop/game.hpp"
#include "playable_level.hpp"
#include <array>
#include <cmath>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace prototype {

class DemoLevel {
public:
    explicit DemoLevel(PlayableLevelDefinition definition) {
        load(std::move(definition));
    }

    void load(PlayableLevelDefinition definition) {
        auto candidate_runtime = build_runtime(definition);
        auto candidate = std::make_unique<ActiveLevel>(
            std::move(definition), std::move(candidate_runtime));
        active_.swap(candidate);
    }

    void load_file(const std::filesystem::path& path) {
        auto definition = load_playable_level_file(path);
        load(std::move(definition));
    }

    void restart() {
        auto candidate_runtime = build_runtime(active_->definition);
        auto candidate = std::make_unique<ActiveLevel>(
            active_->definition, std::move(candidate_runtime));
        active_.swap(candidate);
    }

    void interact() {
        auto& runtime = active_->runtime;
        if (runtime.state != elemental_coop::RoundState::playing) return;

        std::size_t selected_gate = runtime.gates.size();
        std::size_t selected_player = elemental_coop::player_count;
        float nearest = std::numeric_limits<float>::max();
        for (std::size_t gate_index = 0; gate_index < active_->definition.gates.size(); ++gate_index) {
            const auto& lever = active_->definition.gates[gate_index].lever;
            const float lever_x = lever.x + lever.w / 2.0F;
            const float lever_y = lever.y + lever.h;
            for (std::size_t player_index = 0; player_index < elemental_coop::player_count; ++player_index) {
                const auto p = position(player_index);
                const float dx = std::abs(p.x - lever_x);
                const float dy = std::abs(p.y - lever_y);
                if (dx >= 60.0F || dy >= 12.0F) continue;
                const float distance = dx * dx + dy * dy;
                if (distance < nearest) {
                    nearest = distance;
                    selected_gate = gate_index;
                    selected_player = player_index;
                }
            }
        }
        if (selected_gate == runtime.gates.size()) return;

        auto& gate_state = runtime.gates[selected_gate];
        const auto& gate_definition = active_->definition.gates[selected_gate];
        if (gate_definition.require_distinct_operators) {
            if (!gate_state.lever_on) {
                gate_state.lever_on = true;
                gate_state.lever_operator = selected_player;
            }
        } else {
            gate_state.lever_on = !gate_state.lever_on;
            gate_state.lever_operator = gate_state.lever_on
                ? std::optional<std::size_t>{selected_player}
                : std::nullopt;
        }
    }

    void tick(const elemental_coop::PlayerInputs& frames, float dt) {
        auto& runtime = active_->runtime;
        if (runtime.state != elemental_coop::RoundState::playing || dt <= 0) return;
        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            auto* body = runtime.world.get<engine::Body>(runtime.players[i].entity);
            body->velocity.x = frames[i].move_x * 240.0F;
            if (frames[i].primary_pressed && runtime.players[i].grounded)
                body->velocity.y = -630.0F;
        }

        runtime.physics.step(runtime.world, dt);
        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            auto& player = runtime.players[i];
            player.body = *runtime.world.get<engine::Body>(player.entity);
            const auto* contacts = runtime.world.get<engine::ContactState>(player.entity);
            player.grounded = contacts != nullptr && contacts->grounded;
            const auto p = position(i);
            const auto player_box = player_bounds(p);
            for (const auto& hazard : active_->definition.hazards) {
                if (engine::overlaps(player_box, bounds(hazard.box))) touch_hazard(i, hazard.kind);
            }
            if (p.y > active_->definition.bounds.y - 5.0F)
                touch_hazard(i, elemental_coop::Hazard::toxic);
        }
        if (runtime.state != elemental_coop::RoundState::playing) return;

        for (std::size_t gate_index = 0; gate_index < runtime.gates.size(); ++gate_index) {
            auto& gate_state = runtime.gates[gate_index];
            const auto& gate_definition = active_->definition.gates[gate_index];
            gate_state.plate_operator.reset();
            std::optional<std::size_t> first_occupant;
            for (std::size_t player_index = 0; player_index < elemental_coop::player_count; ++player_index) {
                if (engine::overlaps(player_bounds(position(player_index)), bounds(gate_definition.plate))) {
                    if (!first_occupant) first_occupant = player_index;
                    if (!gate_definition.require_distinct_operators ||
                        !gate_state.lever_operator ||
                        *gate_state.lever_operator != player_index) {
                        gate_state.plate_operator = player_index;
                        break;
                    }
                }
            }
            if (!gate_state.plate_operator) gate_state.plate_operator = first_occupant;
            const bool distinct_operators =
                !gate_definition.require_distinct_operators ||
                (gate_state.lever_operator && gate_state.plate_operator &&
                 *gate_state.lever_operator != *gate_state.plate_operator);
            if (!gate_state.open && gate_state.lever_on &&
                gate_state.plate_operator && distinct_operators) {
                gate_state.open = true;
                runtime.world.destroy(runtime.door_entities[gate_index]);
            }
        }

        bool both_at_exits = true;
        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            runtime.players[i].at_exit = all_gates_open() &&
                engine::overlaps(player_bounds(position(i)),
                                 bounds(active_->definition.exits[i].area));
            both_at_exits = both_at_exits && runtime.players[i].at_exit;
        }
        if (both_at_exits) runtime.state = elemental_coop::RoundState::won;
    }

    void touch_hazard(std::size_t i, elemental_coop::Hazard hazard) {
        const bool safe = (i == 0 && hazard == elemental_coop::Hazard::lava) ||
                          (i == 1 && hazard == elemental_coop::Hazard::water);
        if (!safe) {
            active_->runtime.players.at(i).alive = false;
            active_->runtime.state = elemental_coop::RoundState::lost;
        }
    }

    engine::Vec2 position(std::size_t i) const {
        return active_->runtime.world.find(active_->runtime.players.at(i).entity)->position;
    }

    const engine::World& world() const { return active_->runtime.world; }
    const elemental_coop::PlayerState& player(std::size_t i) const {
        return active_->runtime.players.at(i);
    }
    elemental_coop::RoundState state() const { return active_->runtime.state; }
    const PlayableLevelDefinition& definition() const { return active_->definition; }
    std::size_t gate_count() const { return active_->runtime.gates.size(); }
    bool gate_open(std::size_t i) const { return active_->runtime.gates.at(i).open; }
    bool gate_lever_on(std::size_t i) const { return active_->runtime.gates.at(i).lever_on; }
    bool gate_plate_occupied(std::size_t i) const {
        return active_->runtime.gates.at(i).plate_operator.has_value();
    }
    bool all_gates_open() const {
        for (const auto& gate : active_->runtime.gates) {
            if (!gate.open) return false;
        }
        return !active_->runtime.gates.empty();
    }
    bool lever() const {
        for (const auto& gate : active_->runtime.gates) {
            if (gate.lever_on) return true;
        }
        return false;
    }
    bool plate() const {
        for (const auto& gate : active_->runtime.gates) {
            if (gate.plate_operator) return true;
        }
        return false;
    }
    bool open() const { return all_gates_open(); }

private:
    struct GateState {
        bool lever_on{};
        bool open{};
        std::optional<std::size_t> lever_operator;
        std::optional<std::size_t> plate_operator;
    };

    struct Runtime {
        Runtime() : physics([] {
            engine::PhysicsSettings settings;
            settings.gravity = {0, 1000};
            settings.max_speed = {260, 900};
            return settings;
        }()) {}

        engine::World world;
        engine::PhysicsSystem physics;
        std::array<elemental_coop::PlayerState, elemental_coop::player_count> players{};
        std::vector<GateState> gates;
        std::vector<engine::EntityId> door_entities;
        elemental_coop::RoundState state{elemental_coop::RoundState::playing};
    };

    struct ActiveLevel {
        ActiveLevel(PlayableLevelDefinition value, Runtime state)
            : definition(std::move(value)), runtime(std::move(state)) {}
        PlayableLevelDefinition definition;
        Runtime runtime;
    };

    static engine::Aabb player_bounds(engine::Vec2 p) {
        return {{p.x - 14.0F, p.y - 46.0F}, {28.0F, 46.0F}};
    }

    static engine::EntityId solid(Runtime& runtime, Rect rectangle) {
        engine::Transform transform;
        transform.position = {rectangle.x, rectangle.y};
        const auto id = runtime.world.create(transform);
        runtime.world.add<engine::Collider>(id, {{0, 0}, {rectangle.w, rectangle.h}});
        runtime.physics.add(runtime.world, id);
        return id;
    }

    static Runtime build_runtime(const PlayableLevelDefinition& definition) {
        Runtime runtime;
        for (const auto& platform : definition.platforms) solid(runtime, platform);
        solid(runtime, {-30, 0, 30, definition.bounds.y});
        solid(runtime, {definition.bounds.x, 0, 30, definition.bounds.y});

        runtime.gates.resize(definition.gates.size());
        runtime.door_entities.reserve(definition.gates.size());
        for (const auto& gate : definition.gates)
            runtime.door_entities.push_back(solid(runtime, gate.door));

        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            engine::Transform transform;
            transform.position = definition.spawns[i];
            const auto id = runtime.world.create(transform);
            runtime.world.add<engine::Body>(id);
            runtime.world.add<engine::Collider>(id, {{-14, -46}, {28, 46}});
            runtime.physics.add(runtime.world, id);
            runtime.players[i] = {id,
                i == 0 ? elemental_coop::Element::fire : elemental_coop::Element::water,
                {}, true, true, false};
        }
        return runtime;
    }

    std::unique_ptr<ActiveLevel> active_;
};

} // namespace prototype
