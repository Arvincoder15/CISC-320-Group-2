#include "engine/interaction.hpp"
namespace engine {
InputFrame InputBuffer::consume() {
    const auto frame = pending_;
    pending_ = {};
    return frame;
}
}
