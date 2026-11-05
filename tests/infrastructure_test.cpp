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
    // Loader exceptions are not cached: the next request retries.
    int attempts = 0;
    engine::ResourceCache<int> retry([&](const std::string&) {
        if (++attempts == 1) throw std::runtime_error("temporary load failure");
        return std::make_shared<int>(7);
    });
    CHECK(throws_as<std::runtime_error>([&] { retry.load("retry"); }));
    CHECK(retry.size() == 0);
    CHECK(*retry.load("retry") == 7 && attempts == 2);
    // Model an SDL-style deleter: cleanup happens exactly once, after all owners release.
    int releases = 0;
    std::shared_ptr<int> external;
    {
        engine::ResourceCache<int> owned([&](const std::string&) {
            return std::shared_ptr<int>(new int(9), [&](int* value) { ++releases; delete value; });
        });
        external = owned.load("shared");
        auto second = owned.load("shared");
        CHECK(second == external);
        owned.clear();
        CHECK(releases == 0);
    }
    CHECK(releases == 0);
    external.reset();
    CHECK(releases == 1);
    CHECK(throws_as<std::invalid_argument>([] { engine::ResourceCache<int> invalid({}); }));
    std::ostringstream output;
    engine::Logger logger(output);
    logger.write(engine::LogLevel::warning, "missing asset");
    CHECK(output.str() == "[WARNING] missing asset\n");
    output.str("");
    logger.set_minimum_level(engine::LogLevel::warning);
    CHECK(logger.minimum_level() == engine::LogLevel::warning);
    logger.write(engine::LogLevel::debug, "resources", "hidden debug");
    logger.write(engine::LogLevel::info, "resources", "hidden info");
    logger.write(engine::LogLevel::warning, "resources", "fallback");
    logger.write(engine::LogLevel::error, "configuration", "bad setting");
    CHECK(output.str() == "[WARNING][resources] fallback\n[ERROR][configuration] bad setting\n");
    logger.set_minimum_level(engine::LogLevel::debug);
    logger.write(engine::LogLevel::debug, "visible");
    CHECK(output.str().ends_with("[DEBUG] visible\n"));
    std::ostringstream quiet_output;
    engine::Logger quiet(quiet_output, engine::LogLevel::error);
    quiet.write(engine::LogLevel::warning, "hidden");
    CHECK(quiet_output.str().empty());
}); }
