#include "test_support.hpp"
#include <enet/enet.h>
// shortcut: smoke test only proves ENet builds and links on each platform; replaced by Session tests in NET-04.
int main() { return run_test([] {
    CHECK(enet_initialize() == 0);
    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = 0; // OS picks a free port
    ENetHost* host = enet_host_create(&address, 1, 2, 0, 0);
    CHECK(host != nullptr);
    CHECK(host->address.port != 0);
    enet_host_destroy(host);
    enet_deinitialize();
}); }
