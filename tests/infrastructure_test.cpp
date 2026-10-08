#include "engine/infrastructure.hpp"
#include "test_support.hpp"
#include <sstream>
int main() { return run_test([] {
    int loads = 0;
    engine::ResourceCache<int> cache([&](const std::string& key) -> std::shared_ptr<int> {
        ++loads;
        if (key == "missing") return nullptr;
        return std::make_shared<int>(42);
    });
    auto handle = cache.load("asset");
    CHECK(cache.load("asset") == handle && loads == 1);
    CHECK(throws_as<std::runtime_error>([&] { cache.load("missing"); }));
    CHECK(cache.size() == 1);
    CHECK(throws_as<std::invalid_argument>([&] { cache.load(""); }));
    std::weak_ptr<int> lifetime = handle;
    cache.clear();
    CHECK(cache.size() == 0 && !lifetime.expired());
    handle.reset();
    CHECK(lifetime.expired());
    cache.load("asset");
    CHECK(loads == 3);
    std::ostringstream output;
    engine::Logger logger(output);
    logger.write(engine::LogLevel::warning, "missing asset");
    CHECK(output.str() == "[WARNING] missing asset\n");
}); }
