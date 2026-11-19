#include "engine/sdl_renderer.hpp"
#include "test_support.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <filesystem>
#include <chrono>
#include <memory>
namespace {
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
struct Temp {
    std::filesystem::path path=std::filesystem::temp_directory_path()/
        ("group2-render-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp() { std::filesystem::create_directory(path); }
    ~Temp() { std::error_code ec; std::filesystem::remove_all(path,ec); }
};
engine::RenderColor pixel(SDL_Surface* s,int x,int y) {
    engine::RenderColor c;
    CHECK(SDL_ReadSurfacePixel(s,x,y,&c.r,&c.g,&c.b,&c.a)); return c;
}
}
int main(int, char**) { return run_test([] {
    Temp temp;
    engine::SdlRendererOptions options;
    options.width=64; options.height=64; options.hidden=true; options.vsync=false;
    options.background={0,0,0,255};
    Surface atlas(SDL_CreateSurface(4,2,SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
    CHECK(atlas);
    CHECK(SDL_FillSurfaceRect(atlas.get(),nullptr,SDL_MapSurfaceRGB(atlas.get(),255,0,0)));
    SDL_Rect right{2,0,2,2};
    CHECK(SDL_FillSurfaceRect(atlas.get(),&right,SDL_MapSurfaceRGB(atlas.get(),0,255,0)));
    auto bmp=temp.path/"atlas.bmp";
    CHECK(SDL_SaveBMP(atlas.get(),bmp.string().c_str()));
    for(int cycle=0;cycle<2;++cycle) {
        engine::SdlRenderer renderer(options);
        renderer.load_texture("atlas",bmp);
        renderer.load_texture("alias",temp.path/"."/"atlas.bmp");
        CHECK(renderer.texture_count()==1);
        CHECK(throws_as<std::filesystem::filesystem_error>([&]{renderer.load_texture("missing",temp.path/"absent.bmp");}));
        CHECK(renderer.texture_count()==1);
        engine::Camera camera; camera.viewport={64,64};camera.position={10,10};camera.zoom=2;
        renderer.set_camera(camera);
        engine::SpriteDraw sprite;
        sprite.position={12,12};sprite.size={8,8};sprite.asset_key="atlas";
        sprite.source=engine::RenderRect{2,0,2,2};sprite.layer=2;
        engine::SpriteDraw background;
        background.position={0,0};background.size={64,64};background.tint={0,0,255,255};background.screen_space=true;
        auto capture=temp.path/"frame.bmp";
        renderer.capture_next_frame(capture);
        const std::vector<engine::SpriteDraw> draws{sprite,background}; // deliberately unsorted
        renderer.draw(draws);
        Surface frame(SDL_LoadBMP(capture.string().c_str()),SDL_DestroySurface);
        CHECK(frame);
        CHECK(pixel(frame.get(),5,5).g==255);
        CHECK(pixel(frame.get(),21,21).b==255);
        sprite.asset_key="absent";
        CHECK(throws_as<std::invalid_argument>([&]{renderer.draw(std::span(&sprite,1));}));
        sprite.asset_key="atlas";sprite.source=engine::RenderRect{3,0,2,2};
        CHECK(throws_as<std::invalid_argument>([&]{renderer.draw(std::span(&sprite,1));}));
        // Same-layer submission order, alpha blending, and tint reset on a cached texture.
        sprite.source=engine::RenderRect{0,0,2,2};sprite.screen_space=true;sprite.position={0,0};
        sprite.size={16,16};sprite.tint={128,255,255,255};sprite.layer=0;
        auto top=sprite;top.position={16,0};top.tint={255,255,255,255};
        renderer.capture_next_frame(capture);
        renderer.draw(std::vector<engine::SpriteDraw>{background,sprite,top});
        frame.reset(SDL_LoadBMP(capture.string().c_str()));
        CHECK(pixel(frame.get(),2,2).r==128);
        CHECK(pixel(frame.get(),18,2).r==255);
        CHECK(pixel(frame.get(),40,40).b==255);
        // Full atlas flipped horizontally must put green on the left.
        sprite.source.reset();sprite.flip_x=true;sprite.tint={255,255,255,255};
        renderer.capture_next_frame(capture);
        renderer.draw(std::span(&sprite,1));
        frame.reset(SDL_LoadBMP(capture.string().c_str()));
        CHECK(pixel(frame.get(),2,2).g==255);
        CHECK(pixel(frame.get(),13,2).r==255);
        // 180-degree rotation has the same horizontal exchange for this atlas.
        sprite.flip_x=false;sprite.rotation=180;
        renderer.capture_next_frame(capture);renderer.draw(std::span(&sprite,1));
        frame.reset(SDL_LoadBMP(capture.string().c_str()));
        CHECK(pixel(frame.get(),2,2).g==255);
        sprite.rotation=0;sprite.source=engine::RenderRect{0,0,2,2};sprite.tint.a=128;
        renderer.capture_next_frame(capture);
        renderer.draw(std::vector<engine::SpriteDraw>{background,sprite});
        frame.reset(SDL_LoadBMP(capture.string().c_str()));
        const auto blended=pixel(frame.get(),2,2);
        CHECK(blended.r>=127 && blended.r<=128 && blended.b>=126 && blended.b<=128);
        renderer.draw({});
    }
    // The shared-window adapter preserves existing host pixels and does not own
    // the renderer. The host can draw and present after adapter destruction.
    CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO));
    {
        std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(
            SDL_CreateWindow("host test",64,64,SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
        CHECK(window);
        std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)> host(
            SDL_CreateRenderer(window.get(),nullptr),SDL_DestroyRenderer);
        CHECK(host);
        CHECK(SDL_SetRenderDrawColor(host.get(),0,0,255,255));
        CHECK(SDL_RenderClear(host.get()));
        {
            engine::SdlRenderer adapter(host.get(),{64,64});
            adapter.load_texture("atlas",bmp);
            engine::SpriteDraw sprite; sprite.asset_key="atlas";sprite.size={16,16};
            adapter.submit(std::span(&sprite,1));
            Surface pixels(SDL_RenderReadPixels(host.get(),nullptr),SDL_DestroySurface);
            CHECK(pixels);
            CHECK(pixel(pixels.get(),2,2).r==255);
            CHECK(pixel(pixels.get(),50,50).b==255);
            engine::SpriteDraw overlay;overlay.position={20,20};overlay.size={10,10};
            overlay.tint={255,0,0,128};
            adapter.submit(std::span(&overlay,1));
            pixels.reset(SDL_RenderReadPixels(host.get(),nullptr));
            const auto blended=pixel(pixels.get(),22,22);
            CHECK(blended.r>=127 && blended.r<=128 && blended.b>=126 && blended.b<=128);
            Uint8 r{},g{},b{},a{};SDL_BlendMode mode{};
            CHECK(SDL_GetRenderDrawColor(host.get(),&r,&g,&b,&a));
            CHECK(r==0 && g==0 && b==255 && a==255);
            CHECK(SDL_GetRenderDrawBlendMode(host.get(),&mode));
            CHECK(mode==SDL_BLENDMODE_NONE);
        }
        CHECK(SDL_SetRenderDrawColor(host.get(),255,255,255,255));
        CHECK(SDL_RenderPoint(host.get(),30,30));
        CHECK(SDL_RenderPresent(host.get()));
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}); }
