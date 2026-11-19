#pragma once
#include "engine/rendering.hpp"
#include <filesystem>
#include <memory>
struct SDL_Renderer;
namespace engine {
struct SdlRendererOptions {
    std::string title{"Group 2 renderer"};
    int width{960}, height{540};
    bool hidden{}, vsync{true};
    RenderColor background{15, 23, 42, 255};
};
// Owns its SDL video reference, window, renderer and cached textures, in that order.
// Main-thread only. The application owns event polling; draw never consumes events.
// draw clears and presents one frame. SDL handles never escape this object.
class SdlRenderer final : public Renderer {
public:
    explicit SdlRenderer(const SdlRendererOptions& options = {});
    // Host owns the renderer/window and must outlive this adapter. Logical size
    // must match the host's presentation. Does not change host presentation mode.
    SdlRenderer(SDL_Renderer* host_renderer, Vec2 logical_size);
    ~SdlRenderer() override;
    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;
    void load_texture(const std::string& key, const std::filesystem::path& bmp_file);
    void set_camera(const Camera& camera);
    const Camera& camera() const;
    std::size_t texture_count() const;
    void draw(std::span<const SpriteDraw> sprites) override;
    // Draw into the current host frame without clearing or presenting it.
    void submit(std::span<const SpriteDraw> sprites);
    // Captures the next frame before presentation (BMP). Useful for smoke tests.
    void capture_next_frame(const std::filesystem::path& path);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    void render(std::span<const SpriteDraw> sprites, bool complete_frame);
};
}
