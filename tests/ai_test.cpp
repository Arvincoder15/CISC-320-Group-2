#include "engine/ai.hpp"
#include "test_support.hpp"
#include <limits>
int main() { return run_test([] {
    engine::PatrolController patrol;
    CHECK(patrol.direction(5, 0, 10) == 1);
    CHECK(patrol.direction(10, 0, 10) == -1);
    CHECK(patrol.direction(5, 0, 10) == -1);
    CHECK(patrol.direction(0, 0, 10) == 1);
    CHECK(throws_as<std::invalid_argument>([&] { patrol.direction(0, 1, 1); }));
    CHECK(throws_as<std::invalid_argument>([&] { patrol.direction(std::numeric_limits<float>::infinity(), 0, 1); }));
}); }
