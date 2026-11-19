#include "engine/sdl_renderer.hpp"
#include "engine/infrastructure.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <stdexcept>
namespace engine {
namespace {
void require(bool ok, const char* operation) {
    if (!ok) throw std::runtime_error(std::string(operation)+": "+SDL_GetError());
}
struct Video {
    Video() { require(SDL_InitSubSystem(SDL_INIT_VIDEO), "Initialize video"); }
    ~Video() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
};
template<class T, auto Destroy> struct Deleter { void operator()(T* p) const { if(p) Destroy(p); } };
using Window = std::unique_ptr<SDL_Window, Deleter<SDL_Window, SDL_DestroyWindow>>;
using NativeRenderer = std::unique_ptr<SDL_Renderer, Deleter<SDL_Renderer, SDL_DestroyRenderer>>;
using Surface = std::unique_ptr<SDL_Surface, Deleter<SDL_Surface, SDL_DestroySurface>>;
bool finite(Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }
}
struct SdlRenderer::Impl {
    Video video;
    Window window;
    NativeRenderer renderer;
    ResourceCache<SDL_Texture> textures;
    std::unordered_map<std::string, std::string> paths;
    Camera camera;
    RenderColor background;
    std::filesystem::path capture;
    explicit Impl(const SdlRendererOptions& o)
        : textures([this](const std::string& path) {
            Surface surface(SDL_LoadBMP(path.c_str()));
            require(bool(surface), "Load BMP");
            // Magenta is the transparent key for the portable RGB BMP asset format.
            require(SDL_SetSurfaceColorKey(surface.get(), true, SDL_MapSurfaceRGB(surface.get(),255,0,255)), "Set texture color key");
            std::shared_ptr<SDL_Texture> texture(SDL_CreateTextureFromSurface(renderer.get(), surface.get()), SDL_DestroyTexture);
            require(bool(texture), "Create texture");
            require(SDL_SetTextureScaleMode(texture.get(), SDL_SCALEMODE_NEAREST), "Set texture filtering");
            require(SDL_SetTextureBlendMode(texture.get(), SDL_BLENDMODE_BLEND), "Set texture blending");
            return texture;
        }), background(o.background) {
        window.reset(SDL_CreateWindow(o.title.c_str(), o.width, o.height,
            SDL_WINDOW_RESIZABLE | (o.hidden ? SDL_WINDOW_HIDDEN : 0)));
        require(bool(window), "Create window");
        renderer.reset(SDL_CreateRenderer(window.get(), nullptr));
        require(bool(renderer), "Create renderer");
        // Some software drivers cannot synchronize; rendering remains usable.
        if(o.vsync) SDL_SetRenderVSync(renderer.get(), 1);
        require(SDL_SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND), "Set blend mode");
        require(SDL_SetRenderLogicalPresentation(renderer.get(), o.width, o.height,
            SDL_LOGICAL_PRESENTATION_LETTERBOX), "Set logical resolution");
        camera.viewport = {static_cast<float>(o.width), static_cast<float>(o.height)};
    }
};
SdlRenderer::SdlRenderer(const SdlRendererOptions& options) {
    if(options.width <= 0 || options.height <= 0) throw std::invalid_argument("Invalid window size");
    impl_ = std::make_unique<Impl>(options);
}
SdlRenderer::~SdlRenderer() = default;
void SdlRenderer::load_texture(const std::string& key, const std::filesystem::path& path) {
    if(key.empty()) throw std::invalid_argument("Empty texture key");
    const auto canonical = std::filesystem::canonical(path).string();
    if(auto it=impl_->paths.find(key); it!=impl_->paths.end()) {
        if(it->second != canonical) throw std::invalid_argument("Texture key already bound to another file");
        return;
    }
    impl_->textures.load(canonical);
    impl_->paths.emplace(key, canonical);
}
void SdlRenderer::set_camera(const Camera& camera) {
    camera.validate();
    // Viewport is the fixed logical window resolution; resize is letterboxed.
    if(camera.viewport.x != impl_->camera.viewport.x || camera.viewport.y != impl_->camera.viewport.y)
        throw std::invalid_argument("Camera viewport must match logical window size");
    impl_->camera = camera;
}
const Camera& SdlRenderer::camera() const { return impl_->camera; }
std::size_t SdlRenderer::texture_count() const { return impl_->textures.size(); }
void SdlRenderer::capture_next_frame(const std::filesystem::path& path) {
    if(path.empty()) throw std::invalid_argument("Empty capture path");
    impl_->capture = path;
}
void SdlRenderer::draw(std::span<const SpriteDraw> sprites) {
    struct Prepared { const SpriteDraw* draw; std::shared_ptr<SDL_Texture> texture; SDL_FRect dest; };
    std::vector<Prepared> ordered;
    ordered.reserve(sprites.size());
    const auto& camera = impl_->camera;
    // Validate the entire frame before touching the backbuffer.
    for(const auto& draw : sprites) {
        if(!finite(draw.position) || !finite(draw.size) || draw.size.x < 0 || draw.size.y < 0 || !std::isfinite(draw.rotation))
            throw std::invalid_argument("Invalid sprite geometry");
        std::shared_ptr<SDL_Texture> texture;
        Vec2 size = draw.size;
        if(!draw.asset_key.empty()) {
            auto it=impl_->paths.find(draw.asset_key);
            if(it==impl_->paths.end()) throw std::invalid_argument("Unknown texture: "+draw.asset_key);
            texture = impl_->textures.load(it->second);
            float w{}, h{};
            require(SDL_GetTextureSize(texture.get(), &w, &h), "Get texture size");
            if(draw.source) {
                auto s=*draw.source;
                if(!std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.width) || !std::isfinite(s.height) ||
                    s.x<0 || s.y<0 || s.width<=0 || s.height<=0 || s.x+s.width>w || s.y+s.height>h)
                    throw std::invalid_argument("Sprite source outside texture");
                w=s.width; h=s.height;
            }
            if(size.x==0) size.x=w;
            if(size.y==0) size.y=h;
        } else if(draw.source || draw.rotation != 0 || draw.flip_x || draw.flip_y) {
            throw std::invalid_argument("Solid rectangles do not support source, rotation or flipping");
        }
        if(size.x<=0 || size.y<=0) throw std::invalid_argument("Sprite size must be positive");
        const auto pos = draw.screen_space ? draw.position : camera.world_to_screen(draw.position);
        const float zoom = draw.screen_space ? 1 : camera.zoom;
        SDL_FRect dest{pos.x,pos.y,size.x*zoom,size.y*zoom};
        if(!std::isfinite(dest.x) || !std::isfinite(dest.y) || !std::isfinite(dest.w) || !std::isfinite(dest.h))
            throw std::invalid_argument("Sprite transform overflow");
        // Conservative rotated bounds, avoiding edge popping for rotated sprites.
        const float radius = std::hypot(dest.w, dest.h)*0.5F;
        const float cx = dest.x+dest.w*0.5F, cy=dest.y+dest.h*0.5F;
        if(cx+radius<=0 || cy+radius<=0 || cx-radius>=camera.viewport.x || cy-radius>=camera.viewport.y) continue;
        ordered.push_back({&draw,std::move(texture),dest});
    }
    std::stable_sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){ return a.draw->layer < b.draw->layer; });
    auto* r=impl_->renderer.get();
    auto bg=impl_->background;
    require(SDL_SetRenderDrawColor(r,bg.r,bg.g,bg.b,bg.a), "Set clear color");
    require(SDL_RenderClear(r), "Clear frame");
    for(const auto& item : ordered) {
        const auto& d=*item.draw;
        const auto c=d.tint;
        if(item.texture) {
            auto* t=item.texture.get();
            require(SDL_SetTextureColorMod(t,c.r,c.g,c.b), "Set tint");
            require(SDL_SetTextureAlphaMod(t,c.a), "Set alpha");
            SDL_FRect source{};
            if(d.source) source={d.source->x,d.source->y,d.source->width,d.source->height};
            auto flip=static_cast<SDL_FlipMode>((d.flip_x ? SDL_FLIP_HORIZONTAL : 0) | (d.flip_y ? SDL_FLIP_VERTICAL : 0));
            require(SDL_RenderTextureRotated(r,t,d.source ? &source : nullptr,&item.dest,d.rotation,nullptr,flip), "Draw sprite");
        } else {
            require(SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a), "Set rectangle color");
            require(SDL_RenderFillRect(r,&item.dest), "Draw rectangle");
        }
    }
    if(!impl_->capture.empty()) {
        Surface surface(SDL_RenderReadPixels(r,nullptr));
        require(bool(surface), "Read frame");
        require(SDL_SaveBMP(surface.get(),impl_->capture.string().c_str()), "Save frame");
        impl_->capture.clear();
    }
    require(SDL_RenderPresent(r), "Present frame");
}
}
