#pragma once
#include "engine/types.hpp"
#include <span>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <string_view>
namespace engine {
struct RenderRect { float x{}, y{}, width{}, height{}; };
struct RenderColor { std::uint8_t r{255}, g{255}, b{255}, a{255}; };
// Position is the top-left corner; rotation is clockwise about the centre.
// Empty asset keys draw solid rectangles. Zero size selects native texture size.
struct SpriteDraw {
    EntityId entity{invalid_entity}; Vec2 position{}; std::string asset_key;
    Vec2 size{};
    std::optional<RenderRect> source{};
    int layer{};
    RenderColor tint{};
    double rotation{};
    bool flip_x{}, flip_y{}, screen_space{};
};
class Camera {
public:
    Vec2 position{};
    Vec2 viewport{960, 540};
    float zoom{1};
    void validate() const;
    Vec2 world_to_screen(Vec2 point) const;
    Vec2 screen_to_world(Vec2 point) const;
    void follow(Vec2 target);
};
// Sheet frames are row-major. Animation time is independent of draw frequency.
class SpriteAnimation {
public:
    SpriteAnimation(int columns, int frame_count, Vec2 frame_size, double fps, bool loop = true);
    void advance(double seconds);
    void reset() noexcept { elapsed_ = 0; }
    RenderRect source() const;
    int frame() const;
private:
    int columns_, count_;
    Vec2 size_;
    double fps_, elapsed_{};
    bool loop_;
};
// Tile IDs are 1-based; 0 is empty. This deliberately does not parse Tiled files.
std::vector<SpriteDraw> tile_draws(std::span<const unsigned> tiles, int columns,
    Vec2 tile_size, int atlas_columns, const std::string& asset_key,
    Vec2 origin = {}, int layer = 0);
// Fixed-cell printable ASCII atlas, starting at space (32), with 16 columns.
std::vector<SpriteDraw> text_draws(std::string_view text, const std::string& atlas,
    Vec2 position, Vec2 cell_size, float scale = 1, RenderColor tint = {}, int layer = 100);
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
