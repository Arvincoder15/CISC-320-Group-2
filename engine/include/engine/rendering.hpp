#pragma once
#include "engine/types.hpp"
#include <span>
#include <string>
#include <vector>
namespace engine {
struct SpriteDraw { EntityId entity{invalid_entity}; Vec2 position{}; std::string asset_key; };
// Aryaman: implement an SDL3 backend behind this boundary.
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void draw(std::span<const SpriteDraw> sprites) = 0;
};
// Test double only. No window or GPU resources.
class RecordingRenderer final : public Renderer {
public:
    void draw(std::span<const SpriteDraw> sprites) override;
    const std::vector<SpriteDraw>& last_frame() const { return frame_; }
private:
    std::vector<SpriteDraw> frame_;
};
}
