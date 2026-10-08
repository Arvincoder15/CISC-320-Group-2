#include "engine/tools.hpp"
#include "test_support.hpp"
int main() { return run_test([] {
    CHECK(engine::validate({1, {{"player", {0, 0}}}}).empty());
    CHECK(engine::validate({2, {}}).size() == 1);
    CHECK(engine::validate({1, {{"", {0, 0}}}}).size() == 1);
}); }
