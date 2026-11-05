#include "engine/infrastructure.hpp"
namespace engine {
void Logger::write(LogLevel level, std::string_view message) {
    write(level, {}, message);
}
void Logger::write(LogLevel level, std::string_view subsystem, std::string_view message) {
    if (level < minimum_) return;
    std::string_view label = "UNKNOWN";
    switch (level) {
        case LogLevel::debug: label = "DEBUG"; break;
        case LogLevel::info: label = "INFO"; break;
        case LogLevel::warning: label = "WARNING"; break;
        case LogLevel::error: label = "ERROR"; break;
    }
    output_ << '[' << label << ']';
    if (!subsystem.empty()) output_ << '[' << subsystem << ']';
    output_ << ' ' << message << '\n';
}
}
