#pragma once
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
namespace engine {
enum class LogLevel { debug, info, warning, error };
// Arvin: caller owns the stream; it must outlive the logger. Main-thread only.
class Logger {
public:
    explicit Logger(std::ostream& output) : output_(output) {}
    void write(LogLevel level, std::string_view message);
private:
    std::ostream& output_;
};
// Strong cache ownership until clear(); outstanding handles may live longer.
// Loader is injected, so graphics/audio adapters can provide custom deleters.
// Single-threaded starter. Keys must be canonicalized by the caller.
template<class Resource>
class ResourceCache {
public:
    using Handle = std::shared_ptr<Resource>;
    using Loader = std::function<Handle(const std::string&)>;
    explicit ResourceCache(Loader loader) : loader_(std::move(loader)) {
        if (!loader_) throw std::invalid_argument("Resource loader is required");
    }
    Handle load(const std::string& key) {
        if (key.empty()) throw std::invalid_argument("Resource key is empty");
        if (auto it = resources_.find(key); it != resources_.end()) return it->second;
        auto resource = loader_(key);
        if (!resource) throw std::runtime_error("Resource load failed: " + key);
        resources_.emplace(key, resource);
        return resource;
    }
    void clear() { resources_.clear(); }
    std::size_t size() const { return resources_.size(); }
private:
    Loader loader_;
    std::unordered_map<std::string, Handle> resources_;
};
}
