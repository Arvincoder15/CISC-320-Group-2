#include "engine/infrastructure.hpp"
namespace engine {
void Logger::write(LogLevel level, std::string_view message) {
    std::string_view label = "UNKNOWN";
    switch (level) {
        case LogLevel::debug: label = "DEBUG"; break;
        case LogLevel::info: label = "INFO"; break;
        case LogLevel::warning: label = "WARNING"; break;
        case LogLevel::error: label = "ERROR"; break;
    }
    output_ << '[' << label << "] " << message << '\n';
}
}
