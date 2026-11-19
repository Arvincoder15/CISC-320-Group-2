#include "engine/sdl_renderer.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <algorithm>
int main(int argc, char** argv) {
    try {
        bool smoke=false;
        std::filesystem::path capture;
        for(int i=1;i<argc;++i) {
            if(std::string_view(argv[i])=="--smoke-test") smoke=true;
            else if(std::string_view(argv[i])=="--capture" && i+1<argc) capture=argv[++i];
            else throw std::invalid_argument("Usage: rendering_demo [--smoke-test] [--capture frame.bmp]");
        }
        engine::SdlRendererOptions options;
        options.title="ELEMENTAL / Rendering laboratory"; options.hidden=smoke; options.vsync=!smoke;
        engine::SdlRenderer renderer(options);
        const char* base=SDL_GetBasePath();
        if(!base) throw std::runtime_error(SDL_GetError());
        const auto assets=std::filesystem::path(base)/"rendering-assets";
        renderer.load_texture("characters",assets/"characters.bmp");
        renderer.load_texture("tiles",assets/"tiles.bmp");
        renderer.load_texture("font",assets/"font.bmp");
        engine::SpriteAnimation fire(4,4,{32,32},6), water(4,4,{32,32},4);
        bool running=true, paused=false;
        double time=0;
        auto previous=SDL_GetTicksNS();
        int frames=0;
        while(running) {
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type==SDL_EVENT_QUIT) running=false;
                if(event.type==SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                    if(event.key.key==SDLK_ESCAPE) running=false;
                    if(event.key.key==SDLK_SPACE) paused=!paused;
                    if(event.key.key==SDLK_R) renderer.set_camera(engine::Camera{});
                }
            }
            const auto now=SDL_GetTicksNS();
            const double dt=smoke ? 1.0/60 : std::min(0.05,static_cast<double>(now-previous)/1e9);
            previous=now;
            auto camera=renderer.camera();
            auto keys=SDL_GetKeyboardState(nullptr);
            const float movement=static_cast<float>(dt)*240/camera.zoom;
            camera.position.x+=(keys[SDL_SCANCODE_RIGHT]-keys[SDL_SCANCODE_LEFT])*movement;
            camera.position.y+=(keys[SDL_SCANCODE_DOWN]-keys[SDL_SCANCODE_UP])*movement;
            if(keys[SDL_SCANCODE_Q]) camera.zoom=std::max(0.5F,camera.zoom-static_cast<float>(dt));
            if(keys[SDL_SCANCODE_E]) camera.zoom=std::min(2.5F,camera.zoom+static_cast<float>(dt));
            renderer.set_camera(camera);
            if(!paused) { time+=dt; fire.advance(dt); water.advance(dt); }
            std::vector<engine::SpriteDraw> draws;
            auto rect=[&](float x,float y,float w,float h,engine::RenderColor color,int layer,bool screen=false) {
                engine::SpriteDraw d; d.position={x,y}; d.size={w,h};d.tint=color;d.layer=layer;d.screen_space=screen;draws.push_back(d);
            };
            rect(38,122,884,348,{23,39,55,255},-10);
            for(int i=0;i<12;++i) rect(65.0F+i*76,145,2,290,{32,51,66,255},-9);
            std::vector<unsigned> tiles(28*10,0);
            for(int x=0;x<28;++x) tiles[9*28+x]=1;
            for(int x=3;x<9;++x) tiles[6*28+x]=2;
            for(int x=18;x<25;++x) tiles[5*28+x]=2;
            auto layer=engine::tile_draws(tiles,28,{32,32},2,"tiles",{32,128},0);
            draws.insert(draws.end(),layer.begin(),layer.end());
            rect(394,397,80,19,{255,106,62,255},1);
            rect(500,397,80,19,{52,181,232,255},1);
            for(int i=0;i<28;++i) {
                const float phase=static_cast<float>(std::fmod(time*0.55+i/28.0,1.0));
                rect(398+static_cast<float>(i%7)*12,395-phase*55,3,3,
                    {255,159,80,static_cast<std::uint8_t>((1-phase)*200)},2);
                rect(503+static_cast<float>(i%7)*12,397-phase*40,3,3,
                    {95,221,255,static_cast<std::uint8_t>((1-phase)*200)},2);
            }
            engine::SpriteDraw a; a.asset_key="characters"; a.size={64,64}; a.layer=3;
            a.position={188+static_cast<float>(std::sin(time)*35),256}; a.source=fire.source(); draws.push_back(a);
            a.position={662+static_cast<float>(std::cos(time)*32),224}; a.source=water.source();
            a.source->y+=32; a.flip_x=true; draws.push_back(a);
            rect(0,0,960,102,{11,18,31,255},90,true);
            rect(32,26,4,48,{83,227,195,255},91,true);
            auto text=[&](std::string_view s,float x,float y,float scale,engine::RenderColor c) {
                auto commands=engine::text_draws(s,"font",{x,y},{8,12},scale,c);
                draws.insert(draws.end(),commands.begin(),commands.end());
            };
            text("ELEMENTAL",52,22,3,{235,244,251,255});
            text("RENDERING LAB / CISC 320",54,66,1.25F,{126,163,183,255});
            text("SDL3  /  SPRITES  /  TILES  /  CAMERA",508,45,1.2F,{83,227,195,255});
            rect(0,472,960,68,{11,18,31,255},90,true);
            text("ARROWS  PAN     Q/E  ZOOM     SPACE  PAUSE     R  RESET",32,486,1.5F,{190,210,226,255});
            text(paused ? "ANIMATION PAUSED" : "CACHED TEXTURES: 3   |   ANIMATED ATLAS + PARTICLE EFFECTS",32,514,1,{110,146,167,255});
            if(!capture.empty() && (!smoke || frames==59)) { renderer.capture_next_frame(capture); capture.clear(); }
            renderer.draw(draws);
            ++frames;
            if(smoke && frames==60) running=false;
            if(!smoke) SDL_Delay(1);
        }
        std::cout<<"Rendered "<<frames<<" frames; cached textures: "<<renderer.texture_count()<<'\n';
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
