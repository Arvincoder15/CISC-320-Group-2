#pragma once
#include "engine/tools.hpp"
#include <stdexcept>
#include <string_view>

namespace engine {
class LevelImportError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Imports a finite orthogonal Tiled object-layer subset; errors include source context.
LevelDocument import_tiled_json(std::string_view json, std::string_view source = "<memory>");
}
