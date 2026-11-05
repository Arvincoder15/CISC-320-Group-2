#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include <iostream>

int main(int argc, char** argv) {
    engine::Logger logger(std::cout, engine::LogLevel::info);
    if (argc != 2) {
        logger.write(engine::LogLevel::error, "infrastructure", "Usage: infrastructure_demo <asset-directory>");
        return 1;
    }
    try {
        engine::FileResourceManager resources(argv[1]);
        const auto data = resources.load("config/elemental_coop.example.json");
        const auto reused = resources.load("config/./elemental_coop.example.json");
        if (data != reused) return 2;
        const auto settings = engine::load_engine_settings(argv[1], "config/elemental_coop.example.json");
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
