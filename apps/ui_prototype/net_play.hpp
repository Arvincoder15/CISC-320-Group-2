#pragma once
// Henry: online two-player play for the UI (see docs/NETWORKING.md, "For Rohan").
// The host's DemoLevel runs the game; the joiner's DemoLevel copies the host's state every frame.
#include "demo_level.hpp"
#include "elemental_coop/net.hpp"
#include "engine/networking.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
namespace prototype {
inline constexpr std::uint16_t default_port = 7777;

// Lets CoopHost run a DemoLevel and report its state, including the campaign level being played.
class DemoNetLevel final : public elemental_coop::NetLevel {
public:
    DemoNetLevel(DemoLevel& level, const std::size_t& campaign_index) : level_(level), campaign_index_(campaign_index) {}
    void tick(const elemental_coop::PlayerInputs& inputs, float dt) override { level_.tick(inputs, dt); }
    void interact() override { level_.interact(); }
    void restart() override { level_.restart(); }
    elemental_coop::NetSnapshot snapshot(std::uint32_t tick) const override {
        elemental_coop::NetSnapshot s{.tick = tick, .round = level_.state(),
                                      .level = static_cast<std::uint8_t>(campaign_index_)};
        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            const auto& p = level_.player(i);
            const auto at = level_.position(i);
            s.players[i] = {at.x, at.y, p.body.velocity.x, p.body.velocity.y, p.grounded, p.alive, p.at_exit,
                            level_.player_facing_left(i)};
        }
        for (std::size_t i = 0; i < level_.gate_count(); ++i)
            s.gates.push_back({level_.gate_lever_on(i), level_.gate_plate_occupied(i), level_.gate_open(i)});
        for (std::size_t i = 0; i < level_.enemies().size(); ++i) {
            const auto at = level_.enemy_position(i);
            const auto& enemy = level_.enemies()[i];
            s.enemies.push_back({at.x, at.y, enemy.facing_left, enemy.animation_time});
        }
        return s;
    }
private:
    DemoLevel& level_;
    const std::size_t& campaign_index_;
};

// Everything the lobby and game loop need for online play. One instance for the whole app.
class OnlinePlay {
public:
    enum class Role { offline, host, joiner };

    OnlinePlay(DemoLevel& level, std::span<const PlayableLevelDefinition> campaign, std::size_t& campaign_index,
               std::unique_ptr<engine::net::Session> session = engine::net::make_enet_session())
        : level_(level), campaign_(campaign), campaign_index_(campaign_index), session_(std::move(session)),
          net_level_(level, campaign_index) {}

    // Host Session button. The game stays paused until start().
    bool host(std::uint16_t port = default_port) {
        leave();
        if (!session_->host(port)) return false;
        role_ = Role::host;
        host_.emplace(net_level_, *session_);
        host_->request(elemental_coop::ControlKind::pause); // lobby: hold the game until Start Game
        return true;
    }
    // Join Session button with the typed address.
    bool join(const std::string& address, std::uint16_t port = default_port) {
        leave();
        if (!session_->join(address, port)) return false;
        role_ = Role::joiner;
        client_.emplace(*session_);
        return true;
    }
    // Host's Start Game button; does nothing until the other player is connected.
    void start() {
        if (!host_ || !host_->client_ready()) return;
        started_ = true;
        host_->request(elemental_coop::ControlKind::resume);
    }
    // Lever key (E) while online; used on the next update().
    void interact() { interact_ = true; }

    // Call every frame on every screen while active(). Both key sets drive this machine's own character:
    // fire on the host, water on the joiner. Replaces DemoLevel::tick() while online.
    void update(const elemental_coop::PlayerInputs& keys, float dt) {
        const engine::InputFrame local{std::clamp(keys[0].move_x + keys[1].move_x, -1.0F, 1.0F),
                                       keys[0].primary_pressed || keys[1].primary_pressed,
                                       keys[0].quit_requested || keys[1].quit_requested};
        if (host_) {
            if (interact_ && playing() && !host_->paused()) level_.interact();
            host_->update(local, dt);
        } else if (client_) {
            client_->update(local, interact_);
            if (client_->ready() && !client_->paused() && client_->latest()) started_ = true;
            if (const auto& state = client_->latest()) follow(*state);
        }
        interact_ = false;
    }
    // Pause, resume, restart or quit from a button or key; the host applies it to both players.
    void request(elemental_coop::ControlKind kind) {
        if (host_) host_->request(kind);
        if (client_) client_->request(kind);
    }
    // Main Menu / Back: end the session and put the level back for local play.
    void leave() {
        host_.reset();
        client_.reset();
        session_->close();
        if (role_ != Role::offline) level_.restart();
        role_ = Role::offline;
        started_ = false;
        interact_ = false;
    }

    // Why the last host()/join() failed, e.g. "Could not open port 7777".
    const std::string& error() const { return session_->last_error(); }
    std::string room_code() const { return session_->room_code(); }
    Role role() const { return role_; }
    bool active() const { return role_ != Role::offline; }
    bool connected() const { return host_ ? host_->client_ready() : client_ && client_->ready(); }
    // Both players connected and the host has pressed Start Game.
    bool playing() const { return active() && connected() && started_; }
    bool paused() const { return host_ ? host_->paused() : client_ && client_->paused(); }
    bool connection_lost() const {
        if (!active()) return false;
        const auto state = session_->state();
        return (client_ && client_->rejected()) || state == engine::net::SessionState::failed
            || state == engine::net::SessionState::disconnected;
    }
    // One line for the lobby or HUD.
    std::string status() const {
        if (!active()) return "";
        if (client_ && client_->rejected()) return "The host runs a different version of the game";
        if (connection_lost()) return session_->last_error().empty() ? "Connection lost" : session_->last_error();
        if (host_) {
            if (!host_->client_ready()) {
                if (session_->state() == engine::net::SessionState::connecting) return "Creating room...";
                if (const auto code = room_code(); !code.empty()) return "Room code: " + code + "  |  Waiting for Player 2";
                return "Waiting for a player on port " + std::to_string(session_->port());
            }
            return started_ ? "Online: you are FIRE" : "Player 2 connected. Press Start Game";
        }
        if (!client_->ready()) return "Connecting...";
        return started_ ? "Online: you are WATER" : "Connected. Waiting for the host to start";
    }

private:
    void follow(const elemental_coop::NetSnapshot& state) {
        if (state.level != campaign_index_ && state.level < campaign_.size()) {
            campaign_index_ = state.level;
            level_.load(campaign_[campaign_index_]);
        }
        if (state.level == campaign_index_) level_.apply_network_state(state);
    }

    DemoLevel& level_;
    std::span<const PlayableLevelDefinition> campaign_;
    std::size_t& campaign_index_;
    std::unique_ptr<engine::net::Session> session_;
    DemoNetLevel net_level_;
    std::optional<elemental_coop::CoopHost> host_;
    std::optional<elemental_coop::CoopClient> client_;
    Role role_{Role::offline};
    bool started_{false};
    bool interact_{false};
};
}
