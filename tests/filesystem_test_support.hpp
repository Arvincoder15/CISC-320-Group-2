#pragma once
#include <filesystem>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
namespace {
class TestDirectory {
public:
    TestDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int i = 0; i < 100; ++i) {
            path = fs::temp_directory_path() / ("group2-resources-" + std::to_string(stamp) + "-" + std::to_string(i));
            if (fs::create_directory(path)) return;
        }
        throw std::runtime_error("Unable to create test directory");
    }
    ~TestDirectory() { std::error_code error; fs::remove_all(path, error); }
    fs::path path;
};
void write(const fs::path& path, const std::vector<char>& bytes) {
    std::ofstream stream(path, std::ios::binary);
    stream.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    stream.close();
    if (!stream) throw std::runtime_error("Unable to create test fixture");
}
}
