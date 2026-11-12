#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include <iostream>

int main(int argc, char** argv) {
    engine::Logger logger(std::cout, engine::LogLevel::info);
    if (argc != 2 && argc != 3) {
        logger.write(engine::LogLevel::error, "infrastructure", "Usage: infrastructure_demo <asset-directory> [user-settings-file]");
        return 1;
    }
    try {
        engine::FileResourceManager resources(argv[1]);
        const auto data = resources.load("config/elemental_coop.example.json");
        const auto reused = resources.load("config/./elemental_coop.example.json");
        if (data != reused) return 2;
        auto settings = engine::load_engine_settings(argv[1], "config/elemental_coop.example.json");
        if (argc == 3) {
            const auto startup = engine::load_user_settings(argv[2], settings);
            settings = startup.settings;
            if (startup.status == engine::SettingsLoadStatus::failed)
                logger.write(engine::LogLevel::warning, "configuration", startup.diagnostic);
            else
                logger.write(engine::LogLevel::info, "configuration",
                             startup.status == engine::SettingsLoadStatus::loaded
                                 ? "Loaded saved user settings" : "First launch: using default settings");
        }
        logger.write(engine::LogLevel::info, "resources", "Repeated request reused the cached file");
        std::cout << "Window: " << settings.window.width << 'x' << settings.window.height
                  << "; volume: " << settings.audio.master_volume << '\n';
        resources.clear();
        logger.write(engine::LogLevel::info, "resources", "Cache cleared; shared handles still retain file data");
        return 0;
    } catch (const std::exception& error) {
        logger.write(engine::LogLevel::error, "infrastructure", error.what());
        return 1;
    }
}
