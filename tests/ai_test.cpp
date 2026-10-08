#include "engine/ai.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    engine::VerticalSteering steering;
    CHECK(steering.request_upward_action({20, 10, 2}));
    CHECK(!steering.request_upward_action({12, 10, 2}));
    CHECK(!steering.request_upward_action({5, 10, 2}));
    CHECK(throws_as<std::invalid_argument>([&] { steering.request_upward_action({0, 0, -1}); }));
}); }
