#include "engine/rendering.hpp"
namespace engine {
void RecordingRenderer::draw(std::span<const SpriteDraw> sprites) {
    frame_.assign(sprites.begin(), sprites.end());
}
}
