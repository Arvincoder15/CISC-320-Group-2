#include "engine/networking.hpp"
#include "test_support.hpp"
#include <enet/enet.h>
#include <limits>
using namespace engine::net;

namespace {
std::vector<std::byte> bytes_of(std::initializer_list<int> values) {
    std::vector<std::byte> out;
    for (const int v : values) out.push_back(static_cast<std::byte>(v));
    return out;
}
std::vector<Event> poll_n(Session& session, int times) {
    std::vector<Event> all;
    for (int i = 0; i < times; ++i)
        for (auto& event : session.poll()) all.push_back(std::move(event));
    return all;
}
void connect(Session& host, Session& client, int polls = 1) {
    CHECK(host.host(7777));
    CHECK(client.join("loopback", 7777));
    poll_n(host, polls);
    poll_n(client, polls);
    CHECK(host.state() == SessionState::connected && client.state() == SessionState::connected);
}
}

int main() { return run_test([] {
    // Byte codec: little-endian layout and exact round trip.
    ByteWriter writer;
    writer.u8(7).u32(0xA1B2C3D4u).f32(-1.5F);
    const auto& written = writer.bytes();
    CHECK(written.size() == 9);
    CHECK(written[1] == std::byte{0xD4} && written[4] == std::byte{0xA1});
    ByteReader reader(written);
    CHECK(reader.u8() == 7);
    CHECK(reader.u32() == 0xA1B2C3D4u);
    CHECK(reader.f32() == -1.5F);
    CHECK(reader.done());

    // Short input and non-finite floats are rejected (network trust boundary).
    const auto three = bytes_of({1, 2, 3});
    ByteReader short_reader(three);
    CHECK(!short_reader.u32());
    ByteReader empty_reader({});
    CHECK(!empty_reader.u8() && empty_reader.done());
    for (const float bad : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        ByteWriter w;
        w.f32(bad);
        ByteReader r(w.bytes());
        CHECK(!r.f32());
    }

    {   // Joining with nobody hosting fails with a message.
        auto [host, client] = make_loopback_pair();
        CHECK(!client->join("loopback", 7777));
        CHECK(client->state() == SessionState::failed && !client->last_error().empty());
        CHECK(host->state() == SessionState::idle);
    }
    {   // Connect, exchange a reliable message both ways, then close.
        auto [host, client] = make_loopback_pair();
        const auto hello = bytes_of({1, 2});
        CHECK(!client->send(Channel::reliable, hello)); // not connected yet
        CHECK(host->host(7777) && host->state() == SessionState::hosting && host->port() == 7777);
        CHECK(client->join("loopback", 7777) && client->state() == SessionState::connecting);
        const auto host_events = host->poll();
        CHECK(host_events.size() == 1 && host_events[0].kind == Event::Kind::connected);
        CHECK(client->poll().size() == 1 && client->state() == SessionState::connected);

        CHECK(client->send(Channel::reliable, hello));
        const auto received = host->poll();
        CHECK(received.size() == 1 && received[0].kind == Event::Kind::message && received[0].payload == hello);
        CHECK(host->send(Channel::unreliable, hello));
        CHECK(client->poll().size() == 1);

        host->close();
        CHECK(host->state() == SessionState::idle);
        const auto gone = client->poll();
        CHECK(gone.size() == 1 && gone[0].kind == Event::Kind::disconnected);
        CHECK(client->state() == SessionState::disconnected);
        CHECK(!client->send(Channel::reliable, hello));
        CHECK(host->poll().empty());
    }
    {   // 50% loss hits only the unreliable channel; fixed seed makes it repeatable.
        auto [host, client] = make_loopback_pair({0, 50, 42});
        connect(*host, *client);
        const auto payload = bytes_of({9});
        for (int i = 0; i < 1000; ++i) CHECK(client->send(Channel::unreliable, payload));
        const auto unreliable = host->poll().size();
        CHECK(unreliable >= 400 && unreliable <= 600);
        for (int i = 0; i < 100; ++i) CHECK(client->send(Channel::reliable, payload));
        CHECK(host->poll().size() == 100);
    }
    {   // A delay of 3 polls delivers on the 4th poll, in order.
        auto [host, client] = make_loopback_pair({3, 0, 1});
        connect(*host, *client, 4);
        CHECK(client->send(Channel::reliable, bytes_of({1})));
        CHECK(client->send(Channel::reliable, bytes_of({2})));
        CHECK(poll_n(*host, 3).empty());
        const auto late = host->poll();
        CHECK(late.size() == 2 && late[0].payload == bytes_of({1}) && late[1].payload == bytes_of({2}));
    }

    // shortcut: ENet smoke test only proves ENet builds and links; replaced by Session tests in NET-04.
    CHECK(enet_initialize() == 0);
    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = 0; // OS picks a free port
    ENetHost* enet_host = enet_host_create(&address, 1, 2, 0, 0);
    CHECK(enet_host != nullptr && enet_host->address.port != 0);
    enet_host_destroy(enet_host);
    enet_deinitialize();
}); }
