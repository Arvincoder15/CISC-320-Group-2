#pragma once
#include "engine/infrastructure.hpp"
#include <filesystem>
#include <vector>

namespace engine {
// Raw file data. Format decoding (textures/audio/JSON) belongs to adapters.
struct FileResource {
    std::filesystem::path source;
    std::vector<char> bytes; // Binary-safe: embedded zero bytes are preserved.
};

class ResourceError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Main-thread service. Asset keys are paths relative to an existing asset root.
// Canonical paths deduplicate ./ and symlink aliases; paths outside root fail.
class FileResourceManager {
public:
    using Handle = std::shared_ptr<const FileResource>;
    explicit FileResourceManager(const std::filesystem::path& asset_root);
    Handle load(const std::filesystem::path& key);
    void clear() { cache_.clear(); }
    std::size_t size() const { return cache_.size(); }
    const std::filesystem::path& root() const { return root_; }
private:
    std::filesystem::path resolve(const std::filesystem::path& key) const;
    std::filesystem::path root_;
    ResourceCache<const FileResource> cache_;
};
}
