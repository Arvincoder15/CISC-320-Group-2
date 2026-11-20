// Henry: online play with the real campaign levels, both machines in one process over in-memory links.
#include "net_play.hpp"
#include "engine/room_relay.hpp"
#include "test_support.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <thread>
using namespace prototype;
using elemental_coop::ControlKind;
using elemental_coop::RoundState;
using engine::InputFrame;

namespace {
std::filesystem::path level_asset(const char* name) {
    return std::filesystem::path(UI_PROTOTYPE_ASSET_DIR) / "levels" / name;
}
constexpr float dt = 1.0F / 120.0F; // same step as demo_walkthrough.hpp, so its Level 1 route works

// The state both machines must agree on, without the tick counter.
elemental_coop::NetSnapshot view(DemoLevel& level, std::size_t& index) {
    return DemoNetLevel(level, index).snapshot(0);
}

// Host and joiner, each with its own copy of the campaign level, connected through one in-memory link.
struct Pair {
    const std::array<PlayableLevelDefinition, 3>& campaign;
    DemoLevel host_level, join_level;
    std::size_t host_index = 0, join_index = 0;
    std::optional<OnlinePlay> host, joiner;
    Pair(const std::array<PlayableLevelDefinition, 3>& levels, engine::net::LinkConditions link)
        : campaign(levels), host_level(levels[0]), join_level(levels[0]) {
        auto [host_session, join_session] = engine::net::make_loopback_pair(link);
        host.emplace(host_level, campaign, host_index, std::move(host_session));
        joiner.emplace(join_level, campaign, join_index, std::move(join_session));
    }
    void frame(InputFrame fire = {}, InputFrame water = {}) {
        joiner->update({water, InputFrame{}}, dt);
        host->update({fire, InputFrame{}}, dt);
    }
    void frames(int n) { for (int i = 0; i < n; ++i) frame(); }
    void connect_and_start() {
        CHECK(host->host() && joiner->join("loopback"));
        frames(60);
        CHECK(host->connected() && joiner->connected() && !host->playing());
        host->start();
        frames(60);
        CHECK(host->playing() && joiner->playing());
    }
    // Moves one character like demo_walkthrough.hpp's travel(), but each machine steers only its own
    // character from what it sees: fire from the host's level, water from the joiner's copy.
    void travel(std::size_t who, float target_x, bool jump) {
        DemoLevel& seen = who == 0 ? host_level : join_level;
        bool jump_sent = !jump;
        for (int tick = 0; tick < 960; ++tick) {
            InputFrame input{};
            const float distance = target_x - seen.position(who).x;
            if (std::abs(distance) > 2.5F) input.move_x = distance > 0 ? 1.0F : -1.0F;
            if (!jump_sent && seen.player(who).grounded) input.primary_pressed = jump_sent = true;
            who == 0 ? frame(input, {}) : frame({}, input);
            CHECK(host_level.state() == RoundState::playing);
            if (tick > 2 && seen.player(who).grounded && std::abs(seen.position(who).x - target_x) <= 3.0F) {
                frames(4); // let the last snapshot arrive
                return;
            }
        }
        CHECK(!"travel did not arrive");
    }
};

void check_relay(const std::array<PlayableLevelDefinition, 3>& campaign) {
    engine::net::RoomRelay relay(0);
    const auto session = [&] { return engine::net::make_room_session("127.0.0.1", relay.port()); };
    DemoLevel host_level(campaign[0]), join_level(campaign[0]);
    std::size_t host_index = 0, join_index = 0;
    OnlinePlay host(host_level, campaign, host_index, session());
    OnlinePlay joiner(join_level, campaign, join_index, session());
    auto extra = session();
    const auto pump = [&](auto ready, InputFrame fire = {}, InputFrame water = {}) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
        while (!ready() && std::chrono::steady_clock::now() < deadline) {
            relay.poll();
            host.update({fire, {}}, dt);
            joiner.update({water, {}}, dt);
            (void)extra->poll();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        CHECK(ready());
    };

    CHECK(!extra->join("bad", 0));
    CHECK(host.host());
    pump([&] { return !host.room_code().empty(); });
    const auto code = host.room_code();
    CHECK(host.status().find(code) != std::string::npos && !host.connected());
    CHECK(joiner.join(code));
    pump([&] { return host.connected() && joiner.connected(); });
    CHECK(!host.playing() && !joiner.playing());

    CHECK(extra->join(code, 0));
    pump([&] { return extra->state() == engine::net::SessionState::failed; });
    CHECK(extra->last_error().find("already in use") != std::string::npos);

    host.start();
    pump([&] { return host.playing() && joiner.playing(); });
    const auto initial_fire = host_level.position(0).x;
    const auto initial_water = host_level.position(1).x;
    pump([&] {
        return join_level.position(0).x > initial_fire + 10 &&
               join_level.position(1).x < initial_water - 10 &&
               host_level.position(0).x > initial_fire + 10 &&
               host_level.position(1).x < initial_water - 10;
    }, {1, false, false}, {-1, false, false});

    host.leave();
    pump([&] { return joiner.connection_lost(); });
    CHECK(joiner.status().find("left the room") != std::string::npos);
    joiner.leave();
    CHECK(host.host());
    pump([&] { return !host.room_code().empty(); });
    CHECK(joiner.join(host.room_code()));
    pump([&] { return host.connected() && joiner.connected(); });
}
}

int main(int argc, char** argv) { return run_test([&] {
    const std::array<PlayableLevelDefinition, 3> campaign{
        load_playable_level_file(level_asset("level_1_easy.level.json")),
        load_playable_level_file(level_asset("level_2_medium.level.json")),
        load_playable_level_file(level_asset("level_3_hard.level.json"))};

    if (argc == 2 && std::string_view(argv[1]) == "--relay") {
        check_relay(campaign);
        return;
    }

    {   // Part A, clean link like a LAN: lobby, then the Level 1 lever and plate across the network.
        Pair p(campaign, {});
        CHECK(p.host->status().empty() && !p.host->active());
        CHECK(p.host->host() && p.joiner->join("loopback"));
        CHECK(p.host->status() == "Waiting for a player on port 7777" && p.joiner->status() == "Connecting...");
        for (int i = 0; i < 30; ++i) p.frame(InputFrame{1, false, false}, InputFrame{-1, false, false});
        CHECK(p.host->connected() && p.joiner->connected() && !p.host->playing() && !p.joiner->playing());
        CHECK(p.host->status() == "Player 2 connected. Press Start Game");
        CHECK(p.joiner->status() == "Connected. Waiting for the host to start");
        CHECK(p.host_level.position(0).x == campaign[0].spawns[0].x); // nothing moves in the lobby
        p.host->start();
        p.frames(10);
        CHECK(p.host->playing() && p.joiner->playing() && p.joiner->status() == "Online: you are WATER");
        CHECK(p.host->status() == "Online: you are FIRE");

        p.travel(0, 230, false); // fire, steered on the host
        p.travel(0, 485, true);  // jumps the lava pit to the lever
        p.joiner->interact();    // the joiner presses the lever key; the nearest player (fire) pulls it
        p.frames(6);
        CHECK(p.host_level.gate_lever_on(0) && !p.host_level.gate_open(0));
        CHECK(p.join_level.gate_lever_on(0));
        p.travel(1, 1060, false); // water, steered on the joiner from its own copy
        p.travel(1, 805, true);   // jumps the toxic pit onto the plate
        CHECK(p.host_level.gate_plate_occupied(0) && p.host_level.gate_open(0));
        CHECK(p.join_level.gate_plate_occupied(0) && p.join_level.gate_open(0));
        CHECK(p.join_level.position(1).x == p.host_level.position(1).x); // both stand still now

        // The host moves to Level 2; the joiner loads it by itself.
        p.host_index = 1;
        p.host_level.load(campaign[1]);
        p.frames(10);
        CHECK(p.join_index == 1 && p.join_level.definition().name == campaign[1].name);
    }

    {   // Part B, the NET-5 test link (~100 ms delay, 5% loss): Level 2 with moving enemies.
        Pair p(campaign, {12, 5, 42}); // 12 polls at 120 fps = 100 ms
        p.host_index = 1;
        p.host_level.load(campaign[1]);
        p.connect_and_start();
        CHECK(p.join_index == 1 && p.join_level.enemies().size() == p.host_level.enemies().size());
        CHECK(p.join_level.enemies().size() >= 2);
        p.frames(240); // enemies patrol while both players stand still
        CHECK(p.host_level.state() == RoundState::playing);

        // Paused, the host keeps sending the same state, so both machines end up identical (NFR-18).
        p.joiner->request(ControlKind::pause);
        p.frames(60);
        CHECK(p.host->paused() && p.joiner->paused());
        CHECK(view(p.join_level, p.join_index) == view(p.host_level, p.host_index));
        p.joiner->request(ControlKind::resume);
        p.frames(60);
        CHECK(!p.host->paused() && !p.joiner->paused());

        // The joiner leaves: the host reports the lost connection; the joiner is back to local play.
        p.joiner->leave();
        CHECK(!p.joiner->active() && p.joiner->status().empty());
        for (int i = 0; i < 60; ++i) p.host->update({}, dt);
        CHECK(p.host->connection_lost() && p.host->status() == "Connection lost");
        p.host->leave();
        CHECK(!p.host->active());
    }
}); }
