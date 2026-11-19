#include "engine/rendering.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace engine {
namespace {
bool finite(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
}
void Camera::validate() const {
    if (!finite(position) || !finite(viewport) || viewport.x <= 0 || viewport.y <= 0 ||
        !std::isfinite(zoom) || zoom <= 0) throw std::invalid_argument("Invalid camera");
}
Vec2 Camera::world_to_screen(Vec2 p) const {
    validate();
    return {(p.x-position.x)*zoom, (p.y-position.y)*zoom};
}
Vec2 Camera::screen_to_world(Vec2 p) const {
    validate();
    return {p.x/zoom+position.x, p.y/zoom+position.y};
}
void Camera::follow(Vec2 target) {
    validate();
    if (!finite(target)) throw std::invalid_argument("Invalid camera target");
    position = {target.x-viewport.x/(2*zoom), target.y-viewport.y/(2*zoom)};
}
SpriteAnimation::SpriteAnimation(int columns, int count, Vec2 size, double fps, bool loop)
    : columns_(columns), count_(count), size_(size), fps_(fps), loop_(loop) {
    if (columns <= 0 || count <= 0 || !finite(size) || size.x <= 0 || size.y <= 0 ||
        !std::isfinite(fps) || fps <= 0 || !std::isfinite(count/fps) || count/fps <= 0)
        throw std::invalid_argument("Invalid sprite animation");
}
void SpriteAnimation::advance(double dt) {
    if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("Invalid animation timestep");
    const double duration = count_/fps_;
    if (loop_) elapsed_ = std::fmod(elapsed_ + std::fmod(dt, duration), duration);
    else elapsed_ = std::min(duration, elapsed_ + std::min(dt, duration));
}
int SpriteAnimation::frame() const {
    return static_cast<int>(std::min(static_cast<double>(count_-1), std::floor(elapsed_*fps_)));
}
RenderRect SpriteAnimation::source() const {
    const int f = frame();
    return {static_cast<float>(f%columns_)*size_.x, static_cast<float>(f/columns_)*size_.y, size_.x, size_.y};
}
std::vector<SpriteDraw> tile_draws(std::span<const unsigned> tiles, int columns,
    Vec2 size, int atlas_columns, const std::string& key, Vec2 origin, int layer) {
    if (columns <= 0 || atlas_columns <= 0 || !finite(size) || size.x <= 0 || size.y <= 0 ||
        !finite(origin) || key.empty() || tiles.size()%static_cast<std::size_t>(columns) != 0)
        throw std::invalid_argument("Invalid tile layer");
    std::vector<SpriteDraw> result;
    for (std::size_t i=0; i<tiles.size(); ++i) {
        if (!tiles[i]) continue;
        const unsigned tile = tiles[i]-1;
        SpriteDraw draw;
        draw.position = {origin.x+static_cast<float>(i%columns)*size.x, origin.y+static_cast<float>(i/columns)*size.y};
        draw.size = size; draw.asset_key = key; draw.layer = layer;
        draw.source = RenderRect{static_cast<float>(tile%atlas_columns)*size.x,
            static_cast<float>(tile/atlas_columns)*size.y, size.x, size.y};
        result.push_back(std::move(draw));
    }
    return result;
}
void RecordingRenderer::draw(std::span<const SpriteDraw> sprites) {
    frame_.assign(sprites.begin(), sprites.end());
}
std::vector<SpriteDraw> text_draws(std::string_view text, const std::string& atlas,
    Vec2 position, Vec2 cell, float scale, RenderColor tint, int layer) {
    if(atlas.empty() || !finite(position) || !finite(cell) || cell.x<=0 || cell.y<=0 ||
        !std::isfinite(scale) || scale<=0) throw std::invalid_argument("Invalid bitmap text");
    std::vector<SpriteDraw> result;
    Vec2 cursor=position;
    for(unsigned char c:text) {
        if(c=='\n') { cursor.x=position.x; cursor.y+=cell.y*scale; continue; }
        const unsigned glyph=(c>=32 && c<=126 ? c : '?')-32;
        SpriteDraw draw;
        draw.position=cursor; draw.asset_key=atlas;
        draw.size={cell.x*scale,cell.y*scale};
        draw.source=RenderRect{static_cast<float>(glyph%16)*cell.x,static_cast<float>(glyph/16)*cell.y,cell.x,cell.y};
        draw.layer=layer; draw.tint=tint; draw.screen_space=true;
        result.push_back(std::move(draw)); cursor.x+=cell.x*scale;
    }
    return result;
}
}
