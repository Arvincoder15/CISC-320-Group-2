#include "engine/networking.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::LoopbackTransport transport(2);
    CHECK(!transport.receive());
    CHECK(transport.send({7, 1, {2, 3}}));
    CHECK(transport.send({8, 1, {4, 5}}));
    CHECK(!transport.send({9, 1, {6, 7}}));
    const auto first = transport.receive();
    CHECK(first && first->tick == 7 && first->position.y == 3);
    CHECK(transport.receive()->tick == 8);
    CHECK(!transport.receive());
    CHECK(transport.send({10, 1, {}}));
}); }
