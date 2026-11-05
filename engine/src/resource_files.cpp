#include "engine/resource_files.hpp"
#include <algorithm>
#include <array>
#include <fstream>

namespace engine {
namespace {
ResourceError file_error(const std::filesystem::path& path, const std::string& reason) {
    return ResourceError("Resource '" + path.generic_string() + "': " + reason);
}
std::filesystem::path validate_root(const std::filesystem::path& root) {
    if (root.empty()) throw file_error(root, "asset root is empty");
    std::error_code error;
    const auto canonical = std::filesystem::canonical(root, error);
    if (error) throw file_error(root, "cannot resolve asset root: " + error.message());
    const bool directory = std::filesystem::is_directory(canonical, error);
    if (error || !directory) throw file_error(root, "asset root must be an accessible directory");
    return canonical;
}
std::shared_ptr<const FileResource> read_file(const std::string& key) {
    const std::filesystem::path path(key);
    std::error_code error;
    const bool regular = std::filesystem::is_regular_file(path, error);
    if (error) throw file_error(path, "cannot inspect file: " + error.message());
    if (!regular) throw file_error(path, "expected an existing regular file");
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw file_error(path, "cannot open file for reading");
    auto resource = std::make_shared<FileResource>();
    resource->source = path;
    std::array<char, 8192> buffer{};
    while (stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) || stream.gcount() > 0) {
        resource->bytes.insert(resource->bytes.end(), buffer.begin(), buffer.begin() + stream.gcount());
    }
    if (stream.bad() || !stream.eof()) throw file_error(path, "failed while reading file");
    return resource;
}
}
FileResourceManager::FileResourceManager(const std::filesystem::path& asset_root)
    : root_(validate_root(asset_root)), cache_(read_file) {}

std::filesystem::path FileResourceManager::resolve(const std::filesystem::path& key) const {
    if (key.empty() || key.has_root_path())
        throw file_error(key, "expected a nonempty relative asset path");
    // Reject embedded NULs instead of letting OS APIs silently truncate the path.
    const auto& native = key.native();
    if (native.find(std::filesystem::path::value_type{}) != native.npos)
        throw file_error(key, "asset path contains a null character");
    std::error_code error;
    const auto path = std::filesystem::weakly_canonical(root_ / key, error);
    if (error) throw file_error(root_ / key, "cannot resolve path: " + error.message());
    const auto mismatch = std::mismatch(root_.begin(), root_.end(), path.begin(), path.end());
    if (mismatch.first != root_.end()) throw file_error(key, "asset path escapes root '" + root_.generic_string() + "'");
    return path;
}
FileResourceManager::Handle FileResourceManager::load(const std::filesystem::path& key) {
    return cache_.load(resolve(key).string());
}
}
