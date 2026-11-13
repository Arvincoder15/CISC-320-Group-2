// Rohan's optional SDL3 UI integration prototype. See README.md for scope.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "engine/configuration.hpp"
#include "engine/interaction.hpp"
#include "demo_level.hpp"
#include "font_metrics.hpp"
#include "demo_walkthrough.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr float width = 1280, height = 720;
constexpr SDL_Color navy{9, 26, 47, 255}, panel{20, 49, 77, 255};
constexpr SDL_Color blue{40, 83, 121, 255}, white{237, 246, 255, 255};
constexpr SDL_Color muted{151, 186, 214, 255}, orange{255, 132, 35, 255};
constexpr SDL_Color cyan{37, 195, 237, 255}, green{90, 220, 139, 255};
enum class Screen { menu, lobby, play, pause, settings, lost, won, tools };
struct Button { SDL_FRect rect; std::string label; int id; bool enabled{true}; };
using Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using Audio = std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)>;
struct SdlLifetime {
    SdlLifetime() { if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) throw std::runtime_error(SDL_GetError()); }
    ~SdlLifetime() { SDL_Quit(); }
};

class App {
public:
    explicit App(std::filesystem::path settings_path) : path_(std::move(settings_path)) {
        const auto startup = engine::load_user_settings(path_);
        settings_ = draft_ = startup.settings;
        if (startup.status == engine::SettingsLoadStatus::failed) {
            notice_ = "Saved settings could not load. Defaults are active. See terminal.";
            std::cerr << startup.diagnostic << '\n';
        }
        SDL_Window* w = nullptr;
        SDL_Renderer* r = nullptr;
        if (!SDL_CreateWindowAndRenderer("Elemental Co-op | UI Prototype", settings_.window.width,
                settings_.window.height, SDL_WINDOW_RESIZABLE, &w, &r))
            throw std::runtime_error(SDL_GetError());
        window_.reset(w); renderer_.reset(r);
        SDL_SetRenderLogicalPresentation(r, 1280, 720, SDL_LOGICAL_PRESENTATION_LETTERBOX);
        SDL_SetRenderVSync(r, 1);
        if (!SDL_SetWindowFullscreen(w, settings_.window.fullscreen)) notice_ = "Fullscreen unavailable.";
        SDL_AudioSpec spec{}; spec.format = SDL_AUDIO_F32; spec.channels = 1; spec.freq = 48000;
        audio_.reset(SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr));
        if (audio_) SDL_ResumeAudioStreamDevice(audio_.get());
        else std::cerr << "Audio unavailable: " << SDL_GetError() << '\n';
        load_font();
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        reset_game();
    }
    int run() {
        Uint64 previous = SDL_GetTicks();
        while (running_) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) handle(event);
            const auto now = SDL_GetTicks();
            const float dt = std::min(static_cast<float>(now - previous) / 1000.0F, 0.05F);
            previous = now;
            update(dt);
            draw();
            if (capture_) { screenshot(); capture_ = false; }
            SDL_RenderPresent(renderer_.get());
            SDL_Delay(8);
        }
        return 0;
    }
    // Uses actual event routing and rendering; no sockets or external GUI driver required.
    void smoke_test(const std::filesystem::path& captures) {
        const auto check = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
        draw(); click(640, 245); check(screen_ == Screen::play, "Local Play navigation");
        key(SDL_SCANCODE_D, true); key(SDL_SCANCODE_RIGHT, true);
        update(0.05F); check(position(0).x > 150 && position(1).x > 1130, "Independent keyboard movement");
        key(SDL_SCANCODE_W, true); update(0.05F);
        check(position(0).y < 580 && std::abs(position(1).y-580)<1, "Independent jump");
        SDL_Event focus{}; focus.type = SDL_EVENT_WINDOW_FOCUS_LOST; handle(focus);
        check(screen_ == Screen::pause, "Focus loss pauses");
        check(inputs_[0].consume().move_x == 0 && inputs_[1].consume().move_x == 0, "Focus clears input");
        draw(); click(640, 430); check(screen_ == Screen::settings, "Pause settings navigation");
        draft_.audio.master_volume = 0.3F; draft_.audio.sound_effects = false;
        apply_settings();
        const auto loaded = engine::load_user_settings(path_);
        check(loaded.status == engine::SettingsLoadStatus::loaded && loaded.settings.audio.master_volume == 0.3F,
              "Settings persistence");
        reset_game(); screen_ = Screen::play;
        game_.touch_hazard(0,elemental_coop::Hazard::water); update(0.01F);
        check(screen_ == Screen::lost, "Hazard triggers Game Over");
        reset_game(); screen_ = Screen::play;
        prototype::verification::solve(game_); update(0.01F);
        check(screen_ == Screen::won, "Physical route reaches both exits");
        if (!captures.empty()) {
            std::filesystem::create_directories(captures);
            SDL_SetWindowSize(window_.get(), 1920, 1080);
            SDL_SyncWindow(window_.get());
            const std::array<const char*,8> names{"01-main-menu", "02-lobby", "03-gameplay", "04-pause",
                "05-settings", "06-game-over", "07-level-complete", "08-developer-tools"};
            reset_game(); notice_.clear();
            for (int i=0; i<8; ++i) {
                screen_ = static_cast<Screen>(i); mx_=my_=-1; focus_=0; draw();
                save_frame(captures / (std::string(names[i])+".bmp"));
                SDL_RenderPresent(renderer_.get());
            }
        }
        std::cout << "UI smoke checks passed: keyboard, focus, navigation, settings, loss and win.\n";
    }
private:
    Window window_{nullptr, SDL_DestroyWindow};
    Renderer renderer_{nullptr, SDL_DestroyRenderer};
    Audio audio_{nullptr, SDL_DestroyAudioStream};
    std::filesystem::path path_;
    engine::EngineSettings settings_{}, draft_{};
    prototype::DemoLevel game_;
    std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> font_{nullptr,SDL_DestroyTexture};
    float elapsed_{0};
    std::array<engine::PlayerInput,2> inputs_{};
    Screen screen_{Screen::menu}, previous_{Screen::menu}, tools_return_{Screen::menu};
    bool running_{true}, controls_{false}, capture_{false}, address_active_{false};
    float mx_{-1}, my_{-1};
    int focus_{0}, selected_{0};
    std::string notice_, address_{"127.0.0.1"};
    std::vector<Button> buttons_;
    engine::Vec2 position(std::size_t i) const { return game_.world().find(game_.player(i).entity)->position; }
    void clear_input() { for (auto& p : inputs_) p.clear(); }
    void go(Screen next) {
        clear_input(); screen_ = next; focus_ = 0; controls_ = false;
        address_active_ = false; SDL_StopTextInput(window_.get());
    }
    void reset_game() { clear_input(); game_.restart(); }
    void color(SDL_Color c) { SDL_SetRenderDrawColor(renderer_.get(), c.r,c.g,c.b,c.a); }
    void rect(float x,float y,float w,float h,SDL_Color c) {
        color(c); SDL_FRect box{x,y,w,h}; SDL_RenderFillRect(renderer_.get(), &box);
    }
    void load_font() {
        const auto file=std::filesystem::path(SDL_GetBasePath())/"assets"/"font.bmp";
        std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> source(SDL_LoadBMP(file.string().c_str()),SDL_DestroySurface);
        if(!source) throw std::runtime_error("Missing font asset: "+file.string());
        std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> rgba(SDL_ConvertSurface(source.get(),SDL_PIXELFORMAT_RGBA32),SDL_DestroySurface);
        if(!rgba) throw std::runtime_error(SDL_GetError());
        auto* bytes=static_cast<Uint8*>(rgba->pixels);
        for(int y=0;y<rgba->h;++y) for(int x=0;x<rgba->w;++x) {
            auto* pixel=bytes+y*rgba->pitch+x*4; pixel[3]=pixel[0]; pixel[0]=pixel[1]=pixel[2]=255;
        }
        font_.reset(SDL_CreateTextureFromSurface(renderer_.get(),rgba.get()));
        if(!font_) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(font_.get(),SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(font_.get(),SDL_SCALEMODE_LINEAR);
    }
    float text_width(const std::string& value,float scale) const {
        float total=0; for(unsigned char c:value) total+=glyph_advance[c>=32 && c<127?c-32:31];
        return total*scale/4;
    }
    void text(float x,float y,const std::string& value,float scale=2,SDL_Color c=white) {
        SDL_SetTextureColorMod(font_.get(),c.r,c.g,c.b); SDL_SetTextureAlphaMod(font_.get(),c.a);
        for(unsigned char ch:value) {
            const int n=ch>=32 && ch<127?ch-32:31;
            SDL_FRect src{float(n%16*64),float(n/16*64),64,64};
            SDL_FRect dst{x,y,16*scale,16*scale};
            SDL_RenderTexture(renderer_.get(),font_.get(),&src,&dst);
            x+=glyph_advance[n]*scale/4;
        }
    }
    void center(float y,const std::string& value,float scale=3,SDL_Color c=white) {
        text((width-text_width(value,scale))/2,y,value,scale,c);
    }
    void ellipse(float x,float y,float rx,float ry,SDL_Color c) {
        color(c);
        for(int dy=-int(ry);dy<=int(ry);++dy) {
            const float dx=rx*std::sqrt(std::max(0.0F,1-float(dy*dy)/(ry*ry)));
            SDL_RenderLine(renderer_.get(),x-dx,y+float(dy),x+dx,y+float(dy));
        }
    }
    void circle(float x,float y,float radius,SDL_Color c) { ellipse(x,y,radius,radius,c); }
    void rounded(float x,float y,float w,float h,float radius,SDL_Color c) {
        radius=std::min(radius,std::min(w,h)/2); color(c);
        for(int row=0;row<int(h);++row) {
            const float dy=row<radius?radius-float(row):float(row)>h-radius?float(row)-(h-radius):0;
            const float inset=dy>0?radius-std::sqrt(std::max(0.0F,radius*radius-dy*dy)):0;
            SDL_RenderLine(renderer_.get(),x+inset,y+float(row),x+w-inset,y+float(row));
        }
    }
    void gradient(float x,float y,float w,float h,SDL_Color top,SDL_Color bottom) {
        for(int row=0;row<int(h);++row) {
            const float t=float(row)/h;
            SDL_Color c{Uint8(top.r+(bottom.r-top.r)*t),Uint8(top.g+(bottom.g-top.g)*t),Uint8(top.b+(bottom.b-top.b)*t),255};
            rect(x,y+float(row),w,1,c);
        }
    }
    void polygon(const std::vector<SDL_FPoint>& points,SDL_Color c) {
        float lo=height,hi=0; for(auto p:points) {lo=std::min(lo,p.y);hi=std::max(hi,p.y);}
        color(c);
        for(int y=int(lo);y<=int(hi);++y) {
            std::vector<float> xs;
            for(std::size_t i=0,j=points.size()-1;i<points.size();j=i++) {
                const auto a=points[j],b=points[i];
                if((a.y<=y && b.y>y)||(b.y<=y && a.y>y)) xs.push_back(a.x+(float(y)-a.y)*(b.x-a.x)/(b.y-a.y));
            }
            std::sort(xs.begin(),xs.end());
            for(std::size_t i=0;i+1<xs.size();i+=2) SDL_RenderLine(renderer_.get(),xs[i],float(y),xs[i+1],float(y));
        }
    }
    void character(float x,float feet,bool fire,float size=1) {
        ellipse(x,feet+2*size,28*size,5*size,{5,20,41,110});
        const auto c=fire?SDL_Color{255,106,14,255}:SDL_Color{0,181,241,255};
        rounded(x-15*size,feet-12*size,9*size,13*size,4*size,c);
        rounded(x+6*size,feet-12*size,9*size,13*size,4*size,c);
        std::vector<SDL_FPoint> outline;
        const std::vector<SDL_FPoint> flame{{0,-76},{-12,-68},{-17,-53},{-14,-43},{-25,-52},{-23,-34},{-30,-22},{-24,-12},{-13,-7},{12,-7},{25,-16},{29,-29},{20,-49},{13,-58},{13,-42},{4,-50},{-2,-63}};
        const std::vector<SDL_FPoint> drop{{0,-72},{-9,-58},{-22,-40},{-29,-25},{-26,-14},{-16,-7},{0,-4},{17,-8},{27,-18},{28,-30},{19,-47},{7,-61}};
        for(auto p:fire?flame:drop) outline.push_back({x+p.x*size,feet+p.y*size});
        std::vector<SDL_FPoint> smooth;
        for(std::size_t i=0;i<outline.size();++i) {
            const auto a=outline[(i+outline.size()-1)%outline.size()], b=outline[i];
            const auto c1=outline[(i+1)%outline.size()], d=outline[(i+2)%outline.size()];
            for(int j=0;j<8;++j) {
                const float t=float(j)/8, t2=t*t, t3=t2*t;
                const auto curve=[&](float p0,float p1,float p2,float p3) {
                    return 0.5F*(2*p1+(-p0+p2)*t+(2*p0-5*p1+4*p2-p3)*t2+(-p0+3*p1-3*p2+p3)*t3);
                };
                smooth.push_back({curve(a.x,b.x,c1.x,d.x),curve(a.y,b.y,c1.y,d.y)});
            }
        }
        polygon(smooth,c);
        ellipse(x,feet-27*size,22*size,19*size,fire?SDL_Color{255,176,21,255}:SDL_Color{22,210,249,255});
        if(fire) polygon({{x-17*size,feet-34*size},{x-5*size,feet-60*size},{x+2*size,feet-40*size},{x+13*size,feet-49*size},{x+19*size,feet-27*size}}, {255,176,21,255});
        else ellipse(x-13*size,feet-40*size,4*size,10*size,{109,236,255,180});
        for(float dx:{-8.0F,8.0F}) {
            ellipse(x+dx*size,feet-26*size,3*size,4.5F*size,navy);
            circle(x+(dx-0.6F)*size,feet-28*size,0.9F*size,white);
        }
        color(fire?SDL_Color{194,94,13,255}:SDL_Color{0,126,186,255});
        SDL_RenderLine(renderer_.get(),x-4*size,feet-16*size,x,feet-14*size);
        SDL_RenderLine(renderer_.get(),x,feet-14*size,x+4*size,feet-16*size);
    }
    void button(float x,float y,float w,const std::string& label,int id,bool enabled=true) {
        const SDL_FRect box{x,y,w,60};
        const bool hover=mx_>=x && mx_<=x+w && my_>=y && my_<=y+60;
        const bool selected=static_cast<int>(buttons_.size())==focus_;
        const bool hot=enabled && (hover||selected);
        rounded(x,y+4,w,60,10,{0,12,30,150});
        rounded(x-1,y-1,w+2,62,10,hot?SDL_Color{255,192,36,255}:SDL_Color{67,134,195,255});
        rounded(x,y,w,60,9,hot?SDL_Color{255,118,15,255}:SDL_Color{30,79,126,255});
        if(!enabled) rounded(x,y,w,60,9,{14,32,55,145});
        const float scale=std::min(2.15F,(w-20)*2.15F/std::max(1.0F,text_width(label,2.15F)));
        text(x+(w-text_width(label,scale))/2,y+18,label,scale,enabled?white:muted);
        buttons_.push_back({box,label,id,enabled});
    }
    void tone() {
        if (!audio_ || !settings_.audio.sound_effects) return;
        std::array<float,2400> samples{};
        for(std::size_t i=0;i<samples.size();++i)
            samples[i]=0.12F*settings_.audio.master_volume*std::sin(6.2831853F*660.0F*static_cast<float>(i)/48000.0F)
                *(1.0F-static_cast<float>(i)/static_cast<float>(samples.size()));
        SDL_ClearAudioStream(audio_.get());
        SDL_PutAudioStreamData(audio_.get(),samples.data(),static_cast<int>(sizeof(samples)));
    }
    void apply_settings() {
        try {
            // Validate before live effects. Folder supplied by SDL_GetPrefPath at startup.
            engine::serialize_engine_settings(draft_);
            if (!SDL_SetWindowFullscreen(window_.get(),draft_.window.fullscreen)) throw std::runtime_error(SDL_GetError());
            if (!draft_.window.fullscreen && !SDL_SetWindowSize(window_.get(),draft_.window.width,draft_.window.height))
                throw std::runtime_error(SDL_GetError());
            std::filesystem::create_directories(path_.parent_path());
            engine::save_engine_settings(path_,draft_);
            settings_=draft_; notice_="Settings applied and saved."; tone();
        } catch(const std::exception& error) {
            SDL_SetWindowFullscreen(window_.get(),settings_.window.fullscreen);
            if(!settings_.window.fullscreen) SDL_SetWindowSize(window_.get(),settings_.window.width,settings_.window.height);
            notice_="Could not apply/save settings. Previous settings retained. See terminal.";
            std::cerr << error.what() << '\n';
        }
    }
    void activate(int id) {
        tone();
        switch(id) {
        case 1: reset_game(); go(Screen::play); break;
        case 2: go(Screen::lobby); break;
        case 3: previous_=screen_; draft_=settings_; go(Screen::settings); break;
        case 4: running_=false; break;
        case 5: go(Screen::play); break;
        case 6: go(Screen::menu); break;
        case 7: apply_settings(); break;
        case 8: go(previous_); break;
        case 9: draft_.audio.master_volume=std::max(0.0F,draft_.audio.master_volume-0.1F); break;
        case 10: draft_.audio.master_volume=std::min(1.0F,draft_.audio.master_volume+0.1F); break;
        case 11: draft_.audio.sound_effects=!draft_.audio.sound_effects; break;
        case 12: draft_.window.fullscreen=!draft_.window.fullscreen; break;
        case 13: controls_=!controls_; break;
        case 14: draft_.window.width=draft_.window.width==1280?960:1280;
                 draft_.window.height=draft_.window.width==1280?720:540; break;
        case 15: tools_return_=screen_; go(Screen::tools); break;
        case 16: go(tools_return_); break;
        case 17: selected_=1-selected_; break;
        case 18: reset_game(); notice_="Built-in demo level reloaded."; break;
        case 19: tone(); break;
        case 20: go(Screen::pause); break;
        default: break;
        }
    }
    void click(float x,float y) {
        mx_=x; my_=y;
        if(screen_==Screen::settings && x>=680 && x<=960 && y>=140 && y<=180) { draft_.audio.master_volume=(x-680)/280; return; }
        if(screen_==Screen::lobby && x>=260 && x<1020 && y>=200 && y<=255) {
            address_active_=true; SDL_StartTextInput(window_.get()); return;
        }
        for(const auto& b:buttons_) {
            if(b.enabled && x>=b.rect.x && x<=b.rect.x+b.rect.w && y>=b.rect.y && y<=b.rect.y+b.rect.h) {
                const int id=b.id; activate(id); break;
            }
        }
    }
    void key(SDL_Scancode scan,bool down,bool repeat=false) {
        SDL_Event e{}; e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP;
        e.key.scancode=scan; e.key.repeat=repeat; handle(e);
    }
    void handle(SDL_Event& e) {
        if(e.type==SDL_EVENT_QUIT) { running_=false; return; }
        if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST) {
            clear_input(); if(screen_==Screen::play) go(Screen::pause); return;
        }
        if(e.type==SDL_EVENT_MOUSE_MOTION || e.type==SDL_EVENT_MOUSE_BUTTON_DOWN) {
            SDL_ConvertEventToRenderCoordinates(renderer_.get(),&e);
            if(e.type==SDL_EVENT_MOUSE_MOTION) { mx_=e.motion.x; my_=e.motion.y; }
            else if(e.button.button==SDL_BUTTON_LEFT) click(e.button.x,e.button.y);
            return;
        }
        if(e.type==SDL_EVENT_TEXT_INPUT && screen_==Screen::lobby && address_active_) {
            for(const char* p=e.text.text; *p && address_.size()<40; ++p)
                if((*p>='0' && *p<='9') || (*p>='a' && *p<='z') || *p=='.' || *p==':' || *p=='-') address_+=*p;
            return;
        }
        if(e.type!=SDL_EVENT_KEY_DOWN && e.type!=SDL_EVENT_KEY_UP) return;
        const auto scan=e.key.scancode;
        const bool down=e.type==SDL_EVENT_KEY_DOWN;
        if(down && !e.key.repeat) {
            if(scan==SDL_SCANCODE_F12) { capture_=true; return; }
            if(scan==SDL_SCANCODE_F3) { if(screen_==Screen::tools) go(tools_return_); else { tools_return_=screen_;go(Screen::tools); } return; }
            if(scan==SDL_SCANCODE_ESCAPE) {
                if(screen_==Screen::play) go(Screen::pause);
                else if(screen_==Screen::pause) go(Screen::play);
                else if(screen_==Screen::settings) go(previous_);
                else if(screen_==Screen::tools) go(tools_return_);
                else go(Screen::menu);
                return;
            }
            if(screen_!=Screen::play) {
                if(address_active_ && scan==SDL_SCANCODE_BACKSPACE && !address_.empty()) { address_.pop_back(); return; }
                if(scan==SDL_SCANCODE_TAB || scan==SDL_SCANCODE_DOWN) {
                    if(!buttons_.empty()) focus_=(focus_+1)%static_cast<int>(buttons_.size());
                    return;
                }
                if(scan==SDL_SCANCODE_UP) { if(!buttons_.empty()) focus_=(focus_+static_cast<int>(buttons_.size())-1)%static_cast<int>(buttons_.size()); return; }
                if(scan==SDL_SCANCODE_RETURN && focus_<static_cast<int>(buttons_.size()) && buttons_[focus_].enabled) {
                    activate(buttons_[focus_].id); return;
                }
            }
        }
        if(screen_!=Screen::play || (down && e.key.repeat)) return;
        if(down && scan==SDL_SCANCODE_E) { game_.interact(); return; }
        const std::array<std::array<SDL_Scancode,3>,2> keys{{{SDL_SCANCODE_A,SDL_SCANCODE_D,SDL_SCANCODE_W},
            {SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP}}};
        for(std::size_t p=0;p<2;++p) for(int a=0;a<3;++a) if(scan==keys[p][a]) {
            const auto action=static_cast<engine::PlayerAction>(a);
            if(down) inputs_[p].press(action); else inputs_[p].release(action);
        }
    }
    void update(float dt) {
        elapsed_+=dt;
        if(screen_!=Screen::play || dt<=0) return;
        game_.tick({inputs_[0].consume(),inputs_[1].consume()},dt);
        if(game_.state()==elemental_coop::RoundState::lost) { go(Screen::lost); tone(); }
        if(game_.state()==elemental_coop::RoundState::won) { go(Screen::won); tone(); }
    }
    void background() {
        color(navy); SDL_RenderClear(renderer_.get());
        gradient(0,0,1280,720,{5,31,59,255},{10,65,111,255});
        for(int layer=0;layer<3;++layer) for(int i=0;i<17;++i) {
            const float h=100+float((i*73+layer*91)%320);
            const float x=float(i*85-30);
            rect(x,720-h,65+float(layer*12),h,layer==0?SDL_Color{9,48,84,255}:layer==1?SDL_Color{7,39,70,255}:SDL_Color{5,29,53,255});
        }
        gradient(0,660,1280,60,{5,29,53,255},{4,23,43,255});
        text(30,698,"SDD PROTOTYPE   |   F3  INSPECT   |   F12  SCREENSHOT",0.85F,muted);
    }
    void stone(prototype::Rect r) {
        rect(r.x+3,r.y+7,r.w,r.h,{0,16,32,180});
        const int count=std::max(1,int(r.w/48)); const float tile=r.w/count;
        for(int i=0;i<count;++i) {
            const float x=r.x+float(i)*tile;
            rounded(x+1,r.y,tile-2,r.h,4,{36,68,104,255});
            rect(x+5,r.y+2,tile-10,3,{142,188,224,255});
            rect(x+4,r.y+6,tile-8,3,{66,108,153,255});
            rect(x+tile-4,r.y+8,2,r.h-12,{14,42,71,255});
        }
    }
    void portal(float x,bool fire) {
        const auto c=fire?orange:cyan;
        rounded(x-40,151,80,104,8,{c.r,c.g,c.b,30});
        rounded(x-34,154,68,101,6,c);
        rect(x-25,163,50,92,{9,42,65,255});
        if(game_.open()) { rect(x-24,164,48,90,{c.r,c.g,c.b,70}); }
        text(x-50,118,fire?"FIRE EXIT":"WATER EXIT",1.5F,c);
    }
    void gameplay() {
        text(35,25,"LEVEL 1",3.2F);
        text(395,32,game_.open()?"Door open! Both players reach their exits":"Work together to unlock the centre door",1.65F,white);
        for(const auto& r:game_.platforms) stone(r);
        for(const auto& p:game_.pools) {
            const auto c=p.kind==elemental_coop::Hazard::lava?SDL_Color{255,125,0,255}:p.kind==elemental_coop::Hazard::water?cyan:SDL_Color{87,235,11,255};
            gradient(p.box.x,p.box.y,p.box.w,p.box.h,c,{c.r,Uint8(c.g/2),Uint8(c.b/2),255});
            color(c);
            for(int j=0;j<int(p.box.w);++j) {
                const float y=p.box.y+std::sin(float(j)*0.09F+elapsed_*3)*3;
                SDL_RenderLine(renderer_.get(),p.box.x+j,y,p.box.x+j,p.box.y+5);
            }
            for(int j=0;j<5;++j) circle(p.box.x+12+float(j)*26,p.box.y-5-std::sin(elapsed_*2+j)*5,2,c);
            const std::string label=p.kind==elemental_coop::Hazard::lava?"LAVA":p.kind==elemental_coop::Hazard::water?"WATER":"TOXIC";
            text(p.box.x+(p.box.w-text_width(label,1.6F))/2,655,label,1.6F,c);
        }
        portal(155,true); portal(1125,false);
        const auto d=prototype::DemoLevel::door;
        rounded(d.x-5,d.y-5,d.w+10,d.h+5,4,{112,141,171,255});
        rect(d.x,d.y,d.w,d.h,{25,49,77,255});
        if(!game_.open()) {
            gradient(d.x+6,d.y+6,d.w-12,d.h-6,{108,128,151,255},{58,78,104,255});
            circle(640,320,10,navy); polygon({{635,325},{645,325},{648,343},{632,343}},navy);
        } else { text(610,315,"OPEN",1.4F,cyan); }
        text(608,241,"DOOR",1.6F,white);
        // Lever and plate are drawn at their actual gameplay trigger locations.
        stone({461,568,48,12}); rect(482,542,7,26,{159,182,204,255});
        color(game_.lever()?green:orange);
        SDL_RenderLine(renderer_.get(),485,548,game_.lever()?499:472,526);
        rounded(game_.lever()?490:463,519,18,10,4,game_.lever()?green:orange);
        text(437,655,"SWITCH [E]",1.3F,game_.lever()?green:orange);
        stone({752,570,106,10}); rounded(758,game_.plate()?570:565,94,7,3,game_.plate()?green:cyan);
        text(725,655,"PRESSURE PLATE",1.25F,cyan);
        for(std::size_t i=0;i<2;++i) {
            const auto p=position(i); character(p.x,p.y,i==0,0.85F);
            if(game_.player(i).at_exit) circle(p.x,p.y-72,4,green);
        }
        text(30,79,"FIRE  A / D / W     WATER  ARROWS     E  LEVER",1.1F,muted);
        text(395,79,game_.open()?"Unlocked: climb the stepping stones to the exits.":"Flip the lever, then stand on the pressure plate.",1.1F,cyan);
    }
    void draw() {
        buttons_.clear(); background();
        switch(screen_) {
        case Screen::menu:
            center(80,"ELEMENTAL CO-OP",5.8F); center(165,"Two players. One shared goal.",1.6F,muted);
            button(440,220,400,"LOCAL PLAY",1); button(440,290,400,"MULTIPLAYER",2);
            button(440,360,400,"SETTINGS",3); button(440,430,400,"EXIT",4);
            character(250,510,true,2.3F); character(1030,510,false,2.3F);
            button(440,530,400,"DEVELOPER TOOLS",15); break;
        case Screen::lobby:
            center(65,"MULTIPLAYER LOBBY",4); rounded(139,154,1002,432,12,blue); rounded(141,156,998,428,11,panel);
            text(190,215,"Server",2); rect(310,200,760,55,navy); text(330,219,address_,2);
            text(190,300,"Player 1: Local",2,green); text(190,350,"Player 2: Not connected",2,muted);
            button(190,420,270,"HOST SESSION",0,false); button(505,420,270,"JOIN SESSION",0,false);
            button(820,420,270,"START GAME",0,false);
            center(505,"Network transport is not connected in this prototype.",2,muted);
            button(440,600,400,"BACK",6); break;
        case Screen::play: gameplay(); button(1090,20,155,"PAUSE",20); break;
        case Screen::pause:
            gameplay(); rect(0,0,1280,690,{0,12,29,155}); rounded(389,169,502,456,14,blue); rounded(391,171,498,452,13,panel); center(210,"PAUSED",4);
            button(440,280,400,"RESUME",5); button(440,355,400,"RESTART",1);
            button(440,430,400,"SETTINGS",3); button(440,505,400,"MAIN MENU",6);
            break;
        case Screen::settings: {
            center(50,"SETTINGS",4); rounded(119,114,1042,492,12,blue); rounded(121,116,1038,488,11,panel);
            text(160,150,"Master volume",2); button(570,135,80,"-",9); button(990,135,80,"+",10);
            rounded(680,154,280,16,8,blue); rounded(680,154,std::max(16.0F,280*draft_.audio.master_volume),16,8,cyan); circle(680+280*draft_.audio.master_volume,162,13,white);
            text(775,185,std::to_string(static_cast<int>(std::lround(draft_.audio.master_volume*100)))+"%",2);
            text(160,235,"Sound effects",2); button(570,215,500,draft_.audio.sound_effects?"ON":"OFF",11);
            text(160,315,"Display",2); button(570,295,500,draft_.window.fullscreen?"FULLSCREEN":"WINDOWED",12);
            text(160,395,"Window size",2); button(570,375,500,std::to_string(draft_.window.width)+" x "+std::to_string(draft_.window.height),14);
            button(160,465,400,"VIEW CONTROLS",13); button(670,465,400,"TEST SAVED AUDIO",19,static_cast<bool>(audio_));
            if(controls_) center(550,"Fire: A/D/W | Water: arrows | E: lever | Esc: pause",1.3F,cyan);
            else center(550,"Apply saves preferences. Back discards changes.",1.2F,muted);
            button(270,620,330,"APPLY",7); button(680,620,330,"BACK",8); break;
        }
        case Screen::lost:
        case Screen::won: {
            const bool won=screen_==Screen::won;
            center(70,won?"LEVEL COMPLETE!":"GAME OVER",4);
            center(145,won?"Both players reached their exits":"A character was defeated",2,muted);
            character(470,420,true,3); character(810,420,false,3);
            if(won) { button(100,505,340,"NEXT LEVEL",0,false); button(470,505,340,"REPLAY",1); button(840,505,340,"MAIN MENU",6);
                center(610,"Only one test level is included.",2,muted); }
            else { button(260,505,350,"RETRY",1); button(670,505,350,"MAIN MENU",6); }
            break;
        }
        case Screen::tools: {
            center(50,"DEVELOPER TOOLS",4);
            text(60,120,"LEVEL: reference room 01",2,cyan);
            button(800,105,420,"RELOAD DEMO LEVEL",18);
            rounded(50,190,450,390,10,panel); rounded(530,190,700,390,10,panel);
            text(80,220,"ENTITY LIST",2); button(80,275,380,selected_==0?"> PLAYER FIRE":"> PLAYER WATER",17);
            text(80,370,"2 controllable players",2,muted); text(80,410,"Click to change selection",1.5F,muted);
            const auto p=position(static_cast<std::size_t>(selected_));
            const auto& state=game_.player(static_cast<std::size_t>(selected_));
            text(560,220,"SELECTED ENTITY",2);
            text(560,285,"Position: "+std::to_string(static_cast<int>(p.x))+", "+std::to_string(static_cast<int>(p.y)),2);
            text(560,335,"Velocity: "+std::to_string(static_cast<int>(state.body.velocity.x))+", "+std::to_string(static_cast<int>(state.body.velocity.y)),2);
            text(560,385,state.grounded?"Grounded: yes":"Grounded: no",2);
            text(560,435,"World entities: "+std::to_string(game_.world().size()),2);
            text(560,490,"Simulation frozen while inspecting",1.5F,muted);
            button(440,610,400,"BACK",16); break;
        }
        }
        if(!notice_.empty()) { rect(20,659,1240,23,navy); text(28,663,notice_,1.5F,cyan); }
    }
    void save_frame(const std::filesystem::path& file) {
        std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> surface(SDL_RenderReadPixels(renderer_.get(),nullptr),SDL_DestroySurface);
        if(!surface || !SDL_SaveBMP(surface.get(),file.string().c_str())) throw std::runtime_error(SDL_GetError());
    }
    void screenshot() {
        try {
            const auto dir=path_.parent_path()/"screenshots"; std::filesystem::create_directories(dir);
            const auto file=dir/("screen-"+std::to_string(SDL_GetTicks())+".bmp");
            save_frame(file); std::cout<<"Screenshot: "<<file<<'\n'; notice_="Screenshot saved. See terminal for path.";
        } catch(const std::exception& e) { notice_="Screenshot failed. See terminal."; std::cerr<<e.what()<<'\n'; }
    }
};
} // namespace
int main(int argc,char** argv) {
    try {
        SdlLifetime sdl;
        const bool smoke=argc>1 && std::string(argv[1])=="--smoke-test";
        std::filesystem::path path;
        if(smoke) {
            path=std::filesystem::temp_directory_path()/("elemental-ui-test-"+std::to_string(SDL_GetTicksNS()))/"settings.json";
        } else {
            char* folder=SDL_GetPrefPath("CISC320Group2","ElementalCoopPrototype");
            if(!folder) throw std::runtime_error(SDL_GetError());
            path=std::filesystem::path(folder)/"settings.json"; SDL_free(folder);
        }
        std::cout<<"Settings: "<<path<<'\n';
        App app(path);
        if(smoke) { app.smoke_test(argc>2?std::filesystem::path(argv[2]):std::filesystem::path{}); std::filesystem::remove_all(path.parent_path()); return 0; }
        return app.run();
    } catch(const std::exception& e) { std::cerr<<"UI prototype: "<<e.what()<<'\n'; return 1; }
}
