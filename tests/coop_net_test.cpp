#include "elemental_coop/net.hpp"
#include "test_support.hpp"
#include <limits>
#include <map>
using namespace elemental_coop;
using engine::InputFrame;
using engine::net::Channel;

namespace {
std::vector<std::byte> raw(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    for (const int v : values) out.push_back(static_cast<std::byte>(v));
    return out;
}
// Swap one field in an encoded message; used to build malformed packets from valid ones.
std::vector<std::byte> with_byte(std::vector<std::byte> bytes, std::size_t index, int value) {
    bytes.at(index) = static_cast<std::byte>(value);
    return bytes;
}
// Minimal level for driver tests that only count what the host asks of it.
struct CountingLevel final : NetLevel {
    int ticks = 0, interacts = 0;
    void tick(const PlayerInputs&, float) override { ++ticks; }
    void interact() override { ++interacts; }
    void restart() override {}
    NetSnapshot snapshot(std::uint32_t tick) const override { return NetSnapshot{.tick = tick}; }
};
}

int main() { return run_test([] {
    // Every message survives encode/decode unchanged.
    const NetSnapshot snapshot{.tick = 77, .round = RoundState::won, .level = 2,
        .players = {NetPlayer{1.5F, -2, 3, 4, true, false, true, true}, NetPlayer{5, 6, -7, 8.25F, false, true, false, false}},
        .gates = {NetGate{true, false, false}, NetGate{true, true, true}},
        .enemies = {NetEnemy{100, 200.5F, true, 1.25F}}};
    const NetSnapshot empty_lists{.tick = 1};
    for (const Message& message : {Message{Hello{}}, Message{Welcome{}}, Message{Reject{}},
             Message{NetInput{42, -0.5F, 3, 7}}, Message{snapshot}, Message{empty_lists},
             Message{Control{ControlKind::restart}}}) {
        const auto decoded = decode(encode(message));
        CHECK(decoded && *decoded == message);
    }
    CHECK(encode(empty_lists).size() == 44 && encode(snapshot).size() == 59 && encode(NetInput{}).size() == 12);

    // Malformed packets are rejected without throwing. Snapshot layout: [6] round, [24] fire flags,
    // [42] gate count, [43..44] gate flags, [45] enemy count.
    const auto input = encode(NetInput{1, 0.5F, 0});
    auto longer = input;
    longer.push_back(std::byte{0});
    auto nan_input = encode(NetInput{1, std::numeric_limits<float>::quiet_NaN(), 0});
    auto encoded = encode(snapshot);
    auto snapshot_too_long = encoded;
    snapshot_too_long.push_back(std::byte{0});
    const std::vector<std::byte> snapshot_truncated(encoded.begin(), encoded.end() - 1);
    NetSnapshot too_many_gates{.tick = 1};
    too_many_gates.gates.resize(max_gates + 1);
    for (const auto& bad : {raw({}), raw({1}), with_byte(encode(Hello{}), 0, 1), with_byte(encode(Hello{}), 0, 3),
             raw({2, 99}), longer, std::vector<std::byte>(input.begin(), input.end() - 1), nan_input,
             encode(NetInput{1, 2.0F, 0}), with_byte(encoded, 6, 9), with_byte(encoded, 24, 16),
             with_byte(encoded, 43, 8), with_byte(encoded, 42, 3), with_byte(encoded, 45, 2),
             snapshot_too_long, snapshot_truncated, encode(too_many_gates),
             with_byte(encode(Control{ControlKind::pause}), 2, 0)})
        CHECK(!decode(bad));

    // Host and client over the in-memory pair.
    Game game;
    GameNetLevel level(game);
    auto [host_session, client_session] = engine::net::make_loopback_pair();
    CHECK(host_session->host(7777) && client_session->join("loopback", 7777));
    CoopHost host(level, *host_session);
    CoopClient client(*client_session);
    const auto step = [&](int times, InputFrame host_input, InputFrame client_input) {
        for (int i = 0; i < times; ++i) {
            client.update(client_input);
            host.update(host_input, 1.0F / 60.0F);
            client_input.primary_pressed = false; // presses are one-shot, like InputBuffer
        }
    };
    const auto position = [&](std::size_t slot) { return game.world().find(game.player(slot).entity)->position; };

    step(3, {}, {});
    CHECK(host.client_ready() && client.ready());

    // The joiner moves only the water character (slot 1); the fire character ignores it.
    step(10, {}, InputFrame{1, false, false});
    CHECK(position(1).x > 128 && near(position(0).x, 64));

    // Snapshots reach the client and match the host's game.
    step(1, {}, InputFrame{1, false, false});
    client.update(InputFrame{1, false, false});
    CHECK(client.latest() && client.latest()->tick == host.tick());
    CHECK(near(client.latest()->players[1].x, position(1).x) && near(client.latest()->players[0].x, position(0).x));

    // A stale input packet (old tick, opposite direction) is ignored; garbage changes nothing.
    CHECK(client_session->send(Channel::unreliable, encode(NetInput{1, -1, 0})));
    CHECK(client_session->send(Channel::unreliable, raw({1, 4, 0xFF})));
    host.update({}, 1.0F / 60.0F);
    CHECK(game.player(1).body.velocity.x > 0);

    // One press arrives as exactly one jump.
    step(1, {}, InputFrame{0, true, false});
    CHECK(!game.player(1).grounded && game.player(1).body.velocity.y < 0);

    // Pause from the joiner stops both; resume continues; restart resets both characters.
    client.request(ControlKind::pause);
    step(2, {}, {});
    CHECK(host.paused() && client.paused());
    const auto paused_tick = host.tick();
    const auto frozen = position(1);
    step(5, InputFrame{1, false, false}, InputFrame{1, false, false});
    CHECK(near(position(1).x, frozen.x) && near(position(0).x, 64));
    CHECK(client.latest()->tick == paused_tick); // snapshots keep flowing, tick stands still
    client.request(ControlKind::resume);
    step(2, {}, {});
    CHECK(!host.paused() && !client.paused() && host.tick() > paused_tick);
    host.request(ControlKind::restart);
    step(2, {}, {});
    CHECK(near(position(0).x, 64) && near(position(1).x, 128) && game.state() == RoundState::playing);

    // Quit from the joiner ends the round for both.
    client.request(ControlKind::quit);
    step(2, {}, {});
    CHECK(game.state() == RoundState::quit && client.latest()->round == RoundState::quit);

    {   // A Hello with the wrong protocol version is answered with Reject.
        Game other;
        GameNetLevel other_level(other);
        auto [hs, cs] = engine::net::make_loopback_pair();
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost other_host(other_level, *hs);
        cs->poll();
        CHECK(cs->send(Channel::reliable, with_byte(encode(Hello{}), 0, 1))); // a version-1 joiner
        other_host.update({}, 1.0F / 60.0F);
        CHECK(!other_host.client_ready());
        const auto reply = cs->poll();
        CHECK(reply.size() == 1);
        const auto decoded = decode(reply[0].payload);
        CHECK(decoded && std::holds_alternative<Reject>(*decoded));
    }

    {   // Lost interact (lever key): the packet carrying the press is lost; the next one still interacts once.
        CountingLevel counting;
        auto [hs, cs] = engine::net::make_loopback_pair();
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost counting_host(counting, *hs);
        cs->poll();
        CHECK(cs->send(Channel::reliable, encode(Hello{})));
        counting_host.update({}, 1.0F / 60.0F);
        // NetInput{10, 0, 0, 1} carried the press and was lost, so it is simply never sent.
        for (std::uint32_t t = 11; t < 20; ++t) {
            CHECK(cs->send(Channel::unreliable, encode(NetInput{t, 0, 0, 1})));
            counting_host.update({}, 1.0F / 60.0F);
        }
        CHECK(counting.interacts == 1);
        CHECK(cs->send(Channel::unreliable, encode(NetInput{20, 0, 0, 2})));
        counting_host.update({}, 1.0F / 60.0F);
        CHECK(counting.interacts == 2 && counting.ticks == 11); // ticks once per update after the Hello

        // Through CoopClient: an interact press is counted once per press.
        auto [hs2, cs2] = engine::net::make_loopback_pair();
        CHECK(hs2->host(1) && cs2->join("loopback", 1));
        CountingLevel second;
        CoopHost second_host(second, *hs2);
        CoopClient presser(*cs2);
        for (int i = 0; i < 3; ++i) { presser.update({}); second_host.update({}, 1.0F / 60.0F); }
        presser.update({}, true);
        second_host.update({}, 1.0F / 60.0F);
        for (int i = 0; i < 5; ++i) { presser.update({}); second_host.update({}, 1.0F / 60.0F); }
        CHECK(second.interacts == 1);
    }

    {   // Lost jump: the packet carrying the press never arrives; the next one still jumps exactly once.
        Game other;
        GameNetLevel other_level(other);
        auto [hs, cs] = engine::net::make_loopback_pair();
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost other_host(other_level, *hs);
        cs->poll();
        CHECK(cs->send(Channel::reliable, encode(Hello{})));
        other_host.update({}, 1.0F / 60.0F);
        CHECK(other_host.client_ready());
        // NetInput{10, 0, 1} carried the press and was lost, so it is simply never sent.
        CHECK(cs->send(Channel::unreliable, encode(NetInput{11, 0, 1})));
        other_host.update({}, 1.0F / 60.0F);
        CHECK(!other.player(1).grounded);
        for (std::uint32_t t = 12; t < 80; ++t) { // same count repeated while falling and after landing
            CHECK(cs->send(Channel::unreliable, encode(NetInput{t, 0, 1})));
            other_host.update({}, 1.0F / 60.0F);
        }
        CHECK(other.player(1).grounded); // landed and never jumped again
    }

    // The rest runs on the NET-5 test link: ~100 ms delay (6 polls at 60 fps), 5% unreliable loss, fixed seed.
    const engine::net::LinkConditions bad_link{6, 5, 42};
    struct Session2 {
        Game game;
        GameNetLevel level{game};
        std::unique_ptr<engine::net::Session> host_session, client_session;
        std::optional<CoopHost> host;
        std::optional<CoopClient> client;
        std::map<std::uint32_t, NetSnapshot> host_history; // host state at every tick it reached
        explicit Session2(engine::net::LinkConditions link) {
            std::tie(host_session, client_session) = engine::net::make_loopback_pair(link);
            CHECK(host_session->host(1) && client_session->join("loopback", 1));
            host.emplace(level, *host_session);
            client.emplace(*client_session);
        }
        void step(InputFrame host_input = {}, InputFrame client_input = {}) {
            client->update(client_input);
            host->update(host_input, 1.0F / 60.0F);
            host_history[host->tick()] = snapshot_of(game, host->tick());
        }
    };

    {   // Consistency (NFR-18): every snapshot the joiner holds is exactly the host's state at that tick.
        Session2 s(bad_link);
        for (int i = 0; i < 30; ++i) s.step();
        CHECK(s.host->client_ready() && s.client->ready());
        for (int frame = 0; frame < 300; ++frame) {
            const float direction = (frame / 50) % 2 == 0 ? 1.0F : -1.0F;
            s.step(InputFrame{-direction, frame % 45 == 0, false}, InputFrame{direction, frame % 40 == 0, false});
            if (const auto& seen = s.client->latest()) CHECK(seen->tick > 0 && *seen == s.host_history.at(seen->tick));
        }
        for (int frame = 0; frame < 60; ++frame) s.step();
        const auto& seen = s.client->latest();
        CHECK(seen && *seen == s.host_history.at(seen->tick));
        CHECK(seen->players == snapshot_of(s.game, s.host->tick()).players); // settled: joiner shows the host's world
        CHECK(s.host->tick() - seen->tick <= 10); // about 6 ticks behind on a 100 ms link
    }
    {   // Pause and restart from the joiner arrive through the delay and reach both sides.
        Session2 s(bad_link);
        for (int i = 0; i < 30; ++i) s.step();
        for (int i = 0; i < 60; ++i) s.step({}, InputFrame{1, false, false});
        s.client->request(ControlKind::pause);
        for (int i = 0; i < 20; ++i) s.step();
        CHECK(s.host->paused() && s.client->paused());
        s.client->request(ControlKind::restart);
        s.client->request(ControlKind::resume);
        for (int i = 0; i < 30; ++i) s.step();
        CHECK(!s.host->paused() && !s.client->paused());
        CHECK(s.client->latest()->players[1].x == 128 && s.client->latest()->players[0].x == 64);
    }
    {   // Reconnecting with the same client: a paused host announces the pause, and a new host's low ticks
        // are accepted instead of being treated as stale.
        Game first_game, second_game;
        GameNetLevel first_level(first_game), second_level(second_game);
        auto [hs, cs] = engine::net::make_loopback_pair();
        CoopClient joiner(*cs);
        const auto play = [&](CoopHost& h, int times) {
            for (int i = 0; i < times; ++i) {
                joiner.update({});
                h.update({}, 1.0F / 60.0F);
            }
        };
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost first(first_level, *hs);
        play(first, 200);
        first.request(ControlKind::pause);
        cs->close();
        play(first, 2);
        CHECK(!first.client_ready());
        CHECK(cs->join("loopback", 1));
        play(first, 4);
        CHECK(first.client_ready() && joiner.ready() && joiner.paused());

        cs->close();
        hs->close(); // the host program restarts: new game, ticks start again from zero
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost second(second_level, *hs);
        play(second, 6);
        CHECK(joiner.ready() && !joiner.paused());
        CHECK(joiner.latest() && joiner.latest()->tick < 10 && *joiner.latest() == snapshot_of(second_game, joiner.latest()->tick));
    }
    {   // Disconnect while playing: the host stops simulating once the joiner is gone.
        Session2 s(bad_link);
        for (int i = 0; i < 30; ++i) s.step();
        for (int i = 0; i < 30; ++i) s.step({}, InputFrame{1, false, false});
        s.client_session->close();
        for (int i = 0; i < 20; ++i) s.host->update({}, 1.0F / 60.0F);
        CHECK(!s.host->client_ready());
        const auto stopped = s.host->tick();
        for (int i = 0; i < 20; ++i) s.host->update({}, 1.0F / 60.0F);
        CHECK(s.host->tick() == stopped);
        CHECK(s.host_session->state() == engine::net::SessionState::disconnected);
    }
}); }
