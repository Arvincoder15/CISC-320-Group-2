#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include "test_support.hpp"
#include "filesystem_test_support.hpp"

int main() { return run_test([] {
    using engine::parse_engine_settings;
    const auto defaults = parse_engine_settings(R"({"schema_version":1})");
    CHECK(defaults.window.width == 960 && defaults.window.height == 540 && !defaults.window.fullscreen);
    CHECK(near(defaults.audio.master_volume, 0.8F) && defaults.audio.sound_effects);
    const auto full = parse_engine_settings(R"({"schema_version":1,"window":{"width":1280,"height":720,"fullscreen":true},"audio":{"master_volume":0.25,"sound_effects":false},"game":{"gravity":900}})");
    CHECK(full.window.width == 1280 && full.window.height == 720 && full.window.fullscreen);
    CHECK(near(full.audio.master_volume, 0.25F) && !full.audio.sound_effects);
    CHECK(parse_engine_settings(R"({"schema_version":1,"audio":{"master_volume":0}})").audio.master_volume == 0);
    CHECK(parse_engine_settings(R"({"schema_version":1,"audio":{"master_volume":1}})").audio.master_volume == 1);
    for (const auto* invalid : {
        "", "{", "[]", "{}", R"({"schema_version":2})", R"({"schema_version":1.0})",
        R"({"schema_version":1,"window":null})", R"({"schema_version":1,"audio":[]})",
        R"({"schema_version":1,"window":{"width":0}})", R"({"schema_version":1,"window":{"height":-1}})",
        R"({"schema_version":1,"window":{"width":1.5}})", R"({"schema_version":1,"window":{"width":18446744073709551615}})",
        R"({"schema_version":1,"window":{"fullscreen":"yes"}})", R"({"schema_version":1,"window":{"widht":640}})",
        R"({"schema_version":1,"audio":{"master_volume":1.1}})", R"({"schema_version":1,"audio":{"master_volume":-0.1}})",
        R"({"schema_version":1,"audio":{"master_volume":"loud"}})", R"({"schema_version":1,"audio":{"master_volume":1e400}})",
        R"({"schema_version":1,"audio":{"sound_effects":1}})"
    }) CHECK(throws_as<engine::ConfigurationError>([&] { parse_engine_settings(invalid); }));
    TestDirectory directory;
    const auto file = directory.path / "settings.json";
    const auto save = [&](const std::string& text) { write(file, {text.begin(), text.end()}); };
    save(R"({"schema_version":1,"window":{"width":1200}})");
    auto active = engine::load_engine_settings(directory.path, "settings.json");
    CHECK(active.window.width == 1200);
    save(R"({"schema_version":1,"window":{"width":800},"audio":{"master_volume":2}})");
    bool contextual_error = false;
    try { active = engine::load_engine_settings(directory.path, "settings.json"); }
    catch (const engine::ConfigurationError& error) {
        const std::string message(error.what());
        contextual_error = message.find("settings.json") != std::string::npos && message.find("audio.master_volume") != std::string::npos;
    }
    CHECK(contextual_error && active.window.width == 1200); // No partial application.
    save(R"({"schema_version":1,"window":{"width":800}})");
    active = engine::load_engine_settings(directory.path, "settings.json");
    CHECK(active.window.width == 800); // Reload reads changed file, not cached bytes.
    CHECK(throws_as<engine::ResourceError>([&] { engine::load_engine_settings(directory.path, "absent.json"); }));
}); }
