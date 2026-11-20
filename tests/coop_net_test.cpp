#include "elemental_coop/net.hpp"
#include "test_support.hpp"
#include <limits>
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
}

int main() { return run_test([] {
    // Every message survives encode/decode unchanged.
    const NetSnapshot snapshot{77, RoundState::won,
        {NetPlayer{1.5F, -2, 3, 4, true, false, true}, NetPlayer{5, 6, -7, 8.25F, false, true, false}}};
    for (const Message& message : {Message{Hello{}}, Message{Welcome{}}, Message{Reject{}},
             Message{NetInput{42, -0.5F, 3}}, Message{snapshot}, Message{Control{ControlKind::restart}}}) {
        const auto decoded = decode(encode(message));
        CHECK(decoded && *decoded == message);
    }
    CHECK(encode(snapshot).size() == 41 && encode(NetInput{}).size() == 11);

    // Malformed packets are rejected without throwing.
    const auto input = encode(NetInput{1, 0.5F, 0});
    auto longer = input;
    longer.push_back(std::byte{0});
    auto nan_input = encode(NetInput{1, std::numeric_limits<float>::quiet_NaN(), 0});
    for (const auto& bad : {raw({}), raw({1}), with_byte(encode(Hello{}), 0, 2), raw({1, 99}), longer,
             std::vector<std::byte>(input.begin(), input.end() - 1), nan_input, encode(NetInput{1, 2.0F, 0}),
             with_byte(encode(snapshot), 6, 9), with_byte(encode(snapshot), 23, 8),
             with_byte(encode(Control{ControlKind::pause}), 2, 0)})
        CHECK(!decode(bad));

    // Host and client over the in-memory pair.
    Game game;
    auto [host_session, client_session] = engine::net::make_loopback_pair();
    CHECK(host_session->host(7777) && client_session->join("loopback", 7777));
    CoopHost host(game, *host_session);
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
        auto [hs, cs] = engine::net::make_loopback_pair();
        CHECK(hs->host(1) && cs->join("loopback", 1));
        CoopHost other_host(other, *hs);
        cs->poll();
        CHECK(cs->send(Channel::reliable, with_byte(encode(Hello{}), 0, 2)));
        other_host.update({}, 1.0F / 60.0F);
        CHECK(!other_host.client_ready());
        const auto reply = cs->poll();
        CHECK(reply.size() == 1);
        const auto decoded = decode(reply[0].payload);
        CHECK(decoded && std::holds_alternative<Reject>(*decoded));
    }
}); }
