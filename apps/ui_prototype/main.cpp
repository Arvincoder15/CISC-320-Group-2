// Rohan's optional SDL3 UI integration prototype. See README.md for scope.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "engine/configuration.hpp"
#include "engine/interaction.hpp"
#include "engine/tools.hpp"
#include "engine/resource_files.hpp"
#include "engine/room_relay.hpp"
#include "engine/sdl_renderer.hpp"
#include "demo_level.hpp"
#include "font_metrics.hpp"
#include "demo_walkthrough.hpp"
#include "net_play.hpp"
#include "sound_bank.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {
constexpr float width = 1280, height = 720;
constexpr SDL_Color navy{9, 26, 47, 255}, panel{20, 49, 77, 255};
constexpr SDL_Color blue{40, 83, 121, 255}, white{237, 246, 255, 255};
constexpr SDL_Color muted{151, 186, 214, 255}, orange{255, 132, 35, 255};
constexpr SDL_Color cyan{37, 195, 237, 255}, green{90, 220, 139, 255};
enum class Screen { menu, lobby, play, pause, settings, lost, won, tools };
struct Button { SDL_FRect rect; std::string label; int id; bool enabled{true}; };
struct GameplayCamera {
    float scale{1.0F};
    float offset_x{};
    float offset_y{};
};
using Window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using Renderer = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using prototype::Cue;
struct SdlLifetime {
    SdlLifetime() {
        if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(SDL_GetError());
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) std::cerr << "Audio unavailable: " << SDL_GetError() << '\n';
    }
    ~SdlLifetime() { SDL_Quit(); }
};

std::filesystem::path asset_directory() {
    const char* base = SDL_GetBasePath();
    if (!base) throw std::runtime_error(std::string("Cannot locate prototype assets: ") + SDL_GetError());
    return std::filesystem::path(base) / "assets";
}

std::array<prototype::PlayableLevelDefinition, 3> load_campaign_definitions() {
    const auto levels = asset_directory() / "levels";
    return {
        prototype::load_playable_level_file(levels / "level_1_easy.level.json"),
        prototype::load_playable_level_file(levels / "level_2_medium.level.json"),
        prototype::load_playable_level_file(levels / "level_3_hard.level.json")
    };
}

GameplayCamera gameplay_camera(const std::string& level_name) {
    if (level_name == "Level 3 - Hard") return {0.75F, 160.0F, 150.0F};
    if (level_name == "Level 2 - Medium") return {0.88F, 76.0F, 92.0F};
    return {};
}

class App {
public:
    App(std::filesystem::path settings_path, std::unique_ptr<engine::net::Session> session, bool room_mode)
        : assets_(asset_directory()), path_(std::move(settings_path)),
          campaign_(load_campaign_definitions()), game_(campaign_[0]),
          online_(game_, campaign_, campaign_index_, std::move(session)),
          room_mode_(room_mode) {
        if (room_mode_) address_.clear();
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
        load_font();
        if (SDL_WasInit(SDL_INIT_AUDIO)) {
            audio_.open(assets_);
            audio_.apply(settings_.audio);
        }
        world_renderer_=std::make_unique<engine::SdlRenderer>(r,engine::Vec2{width,height});
        const auto render_assets=asset_directory().parent_path()/"rendering-assets";
        world_renderer_->load_texture("characters",render_assets/"characters.bmp");
        world_renderer_->load_texture("tiles",render_assets/"tiles.bmp");
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        reset_game();
        initialize_level_inspector();
    }
    int run() {
        Uint64 previous = SDL_GetTicks();
        while (running_) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) handle(event);
            const auto now = SDL_GetTicks();
            const auto frame_time = now - previous;
            previous = now;
            update_inspection_metrics(frame_time);
            const float dt = std::min(static_cast<float>(frame_time) / 1000.0F, 0.05F);
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
        if(!captures.empty()) std::filesystem::create_directories(captures);
        const auto check = [](bool ok, const char* message) { if (!ok) throw std::runtime_error(message); };
        check(audio_.device() && SDL_PauseAudioDevice(audio_.device()), "Audio output available for smoke test");
        // With output paused the mixer can't drain the queues between a real input event and its assertion.
        const auto asset_count = 2 + static_cast<std::size_t>(Cue::count);
        check(assets_.size() == asset_count, "Startup cached the font, music and sound effects");
        check(audio_.music_queued() > 0, "Background music starts in the main menu");
        audio_.apply({0.0F, true});
        check(audio_.music_queued() == 0, "Zero master volume clears background music");
        audio_.apply(settings_.audio);
        check(assets_.load("font.bmp") == assets_.load("./font.bmp") && assets_.size() == asset_count,
              "Font requests reuse the cached atlas");
        draw(); click(640, 245); check(screen_ == Screen::play, "Local Play navigation");
        check(audio_.queued(Cue::select) > 0 && audio_.music_queued() > 0,
              "Menu sound and background music can play together");
        key(SDL_SCANCODE_D, true); key(SDL_SCANCODE_RIGHT, true);
        update(0.05F); check(position(0).x > 150 && position(1).x > 1130, "Independent keyboard movement");
        draw();
        check(world_renderer_->texture_count()==2, "Gameplay caches both rendering atlases");
        for(std::size_t i=0;i<2;++i) {
            check(last_player_draws_[i].entity==game_.player(i).entity &&
                  std::abs(last_player_draws_[i].position.x-(position(i).x-27.2F))<0.001F,
                  "Rendered sprite follows the live ECS player");
        }
        update(0.1F);
        check(animations_[0].frame()>0 && animations_[1].frame()>0, "Moving players advance sprite-sheet animation");
        draw();
        if(!captures.empty()) save_frame(captures/"11-level-1-easy.bmp");
        key(SDL_SCANCODE_W, true); update(0.05F);
        check(position(0).y < 580 && std::abs(position(1).y-580)<1, "Independent jump");
        check(audio_.queued(Cue::fire_jump)>0 && audio_.queued(Cue::water_jump)==0, "Fire's jump uses its own sound");
        key(SDL_SCANCODE_UP, true); update(0.016F);
        check(audio_.queued(Cue::water_jump)>0 && audio_.queued(Cue::fire_jump)>0, "Both jump sounds can overlap");
        const auto clear_audio = [this] { audio_.apply({0.0F,false}); audio_.apply(settings_.audio); };
        clear_audio(); update(0.016F);
        check(!audio_.queued(Cue::fire_jump) && !audio_.queued(Cue::water_jump), "Staying airborne is silent");
        SDL_Event focus{}; focus.type = SDL_EVENT_WINDOW_FOCUS_LOST; handle(focus);
        check(screen_ == Screen::pause, "Focus loss pauses");
        check(inputs_[0].consume().move_x == 0 && inputs_[1].consume().move_x == 0, "Focus clears input");
        const auto paused_enemy = game_.enemy_position(0);
        const auto paused_enemy_animation = game_.enemies()[0].animation_time;
        const auto paused_frame=animations_[0].frame();
        update(0.5F);
        check(animations_[0].frame()==paused_frame, "Pause freezes character animation");
        check(game_.enemy_position(0).x == paused_enemy.x &&
              game_.enemies()[0].animation_time == paused_enemy_animation,
              "Pause freezes acid enemy movement and animation");
        draw(); click(640, 430); check(screen_ == Screen::settings, "Pause settings navigation");
        draft_.audio.master_volume=0; draw(); click(860,495);
        check(!audio_.queued(Cue::water_jump), "Zero draft volume mutes preview");
        draft_.audio.master_volume=0.3F; draw(); click(860,495);
        check(audio_.queued(Cue::water_jump)>0 && settings_.audio.master_volume!=draft_.audio.master_volume,
              "Preview uses the draft without saving it");
        draft_.audio.master_volume = 0.3F; draft_.audio.sound_effects = false;
        apply_settings();
        check(audio_.music_queued() > 0, "Disabling sound effects leaves background music playing");
        const auto loaded = engine::load_user_settings(path_);
        check(loaded.status == engine::SettingsLoadStatus::loaded && loaded.settings.audio.master_volume == 0.3F,
              "Settings persistence");
        reset_game(); screen_ = Screen::play;
        game_.touch_hazard(0,elemental_coop::Hazard::water); update(0.01F);
        check(screen_ == Screen::lost, "Hazard triggers Game Over");
        check(!audio_.queued(Cue::defeat) && !audio_.queued(Cue::water_jump), "Applied mute clears and suppresses effects");
        draft_.audio.sound_effects=true; apply_settings();
        auto encounter = campaign_[0];
        const auto acid_spawn = encounter.enemy_spawns[0].position;
        // Start on a separate ledge, then drop onto the slime's floor.
        encounter.platforms.push_back({acid_spawn.x + 48, acid_spawn.y - 50, 48, 12});
        encounter.spawns[0] = {acid_spawn.x + 72, acid_spawn.y - 50};
        game_.load(encounter); screen_ = Screen::play;
        key(SDL_SCANCODE_A, true);
        for (int tick = 0; tick < 24 && screen_ == Screen::play; ++tick) update(0.01F);
        key(SDL_SCANCODE_A, false);
        for (int tick = 0; tick < 180 && screen_ == Screen::play; ++tick) update(0.01F);
        check(screen_ == Screen::lost, "Acid enemy contact routes to Game Over");
        check(audio_.queued(Cue::defeat)>0, "Defeat has its own sound");
        game_.load(campaign_[0]);
        reset_game(); screen_ = Screen::play;
        prototype::verification::solve(game_); update(0.01F);
        check(screen_ == Screen::won, "Physical route reaches both exits");
        check(audio_.queued(Cue::complete)>0, "Easy victory plays the completion cue");
        draw(); click(270, 535);
        check(screen_ == Screen::play && campaign_index_ == 1 &&
              game_.definition().name == "Level 2 - Medium",
              "Next Level advances Easy to Medium");
        const auto has_centered_platform = [this](float center_x) {
            return std::any_of(game_.definition().platforms.begin(),
                               game_.definition().platforms.end(),
                [center_x](const auto& platform) {
                    constexpr float width = 160.0F / 3.0F;
                    return std::abs(platform.y - 340.0F) < 0.001F &&
                        std::abs(platform.w - width) < 0.001F &&
                        std::abs(platform.x + platform.w / 2.0F - center_x) < 0.001F;
                });
        };
        check(has_centered_platform(330.0F) && has_centered_platform(950.0F),
              "Runtime Medium loads one centered upper platform for each player");
        draw();
        if(!captures.empty()) save_frame(captures/"12-level-2-medium.bmp");
        clear_audio(); prototype::verification::solve_medium(game_); update(0.01F);
        check(screen_ == Screen::won && audio_.queued(Cue::complete)>0, "Medium victory screen and sound");
        draw(); click(270, 535);
        check(screen_ == Screen::play && campaign_index_ == 2 &&
              game_.definition().name == "Level 3 - Hard",
              "Next Level advances Medium to Hard");
        clear_audio(); prototype::verification::solve_hard(game_); update(0.01F);
        check(screen_ == Screen::won && campaign_index_ == 2 && audio_.queued(Cue::complete)>0,
              "Hard completion reaches the final campaign state");
        exercise_inspector(check);
        check(gameplay_camera(campaign_[0].name).scale == 1.0F &&
              gameplay_camera(campaign_[1].name).scale < 1.0F &&
              gameplay_camera(campaign_[2].name).scale < gameplay_camera(campaign_[1].name).scale,
              "Medium and Hard camera frames fit their taller levels");
        game_.load(campaign_[2]);
        campaign_index_ = 2;
        screen_ = Screen::play;
        draw();
        const auto camera = gameplay_camera(game_.definition().name);
        const auto project_x = [&camera](float x) {
            return camera.offset_x + camera.scale * x;
        };
        const auto project_y = [&camera](float y) {
            return camera.offset_y + camera.scale * y;
        };
        const auto exit_visible = [&](const auto& exit, const char* label) {
            const float center_x = exit.area.x + exit.area.w / 2.0F;
            const float surface_y = exit.area.y + exit.area.h;
            const float portal_left = project_x(center_x - 40.0F);
            const float portal_right = project_x(center_x + 40.0F);
            const float portal_top = project_y(surface_y - 104.0F);
            const float portal_bottom = project_y(surface_y);
            const float label_left = project_x(center_x - 50.0F);
            const float label_right =
                project_x(center_x - 50.0F + text_width(label, 1.5F));
            const float label_top = project_y(surface_y - 137.0F);
            const float label_bottom = project_y(surface_y - 113.0F);
            return portal_left >= 0.0F && portal_right <= width &&
                portal_top >= 0.0F && portal_bottom <= height &&
                label_left >= 0.0F && label_right <= width &&
                label_top >= 80.0F && label_bottom <= height;
        };
        check(exit_visible(game_.definition().exits[0], "FIRE EXIT") &&
              exit_visible(game_.definition().exits[1], "WATER EXIT"),
              "Hard keeps both complete exit portals and labels in frame");
        // Stage the two actors at the controls, then use the real E binding and physics tick.
        auto puzzle = campaign_[0];
        const auto& gate = puzzle.gates.front();
        puzzle.spawns = {engine::Vec2{gate.lever.x + gate.lever.w/2, gate.lever.y+gate.lever.h},
                         engine::Vec2{gate.plate.x + gate.plate.w/2, gate.plate.y+gate.plate.h}};
        game_.load(puzzle); go(Screen::play); clear_audio();
        key(SDL_SCANCODE_E,true); update(0.016F);
        check(game_.gate_open(0) && audio_.queued(Cue::lever)>0 && audio_.queued(Cue::gate)>0,
              "Switch and gate transitions play distinct sounds");
        clear_audio(); update(0.016F);
        check(!audio_.queued(Cue::lever) && !audio_.queued(Cue::gate), "A latched gate stays silent");
        start_campaign();
        // Exercise lobby controls with loopback. net_session checks real ENet sockets separately.
        go(Screen::menu); draw(); click(640, 315);
        check(screen_ == Screen::lobby, "Multiplayer opens the lobby");
        draw(); click(325, 445);
        check(online_.role() == prototype::OnlinePlay::Role::host &&
              online_.status().rfind("Waiting for a player", 0) == 0, "Host Session starts hosting");
        update(0.01F); draw(); click(955, 445);
        check(screen_ == Screen::lobby && !online_.playing(), "Start Game waits for a second player");
        draw(); click(640, 625);
        check(!online_.active() && screen_ == Screen::menu, "Back leaves the session");
        if (!captures.empty()) {
            std::filesystem::create_directories(captures);
            SDL_SetWindowSize(window_.get(), 1920, 1080);
            SDL_SyncWindow(window_.get());
            draw();
            save_frame(captures / "10-level-3-hard-framing.bmp");
            SDL_RenderPresent(renderer_.get());
            const std::array<const char*,8> names{"01-main-menu", "02-lobby", "03-gameplay", "04-pause",
                "05-settings", "06-game-over", "07-level-complete", "08-developer-tools"};
            reset_game(); notice_.clear();
            for (int i=0; i<8; ++i) {
                screen_ = static_cast<Screen>(i); mx_=my_=-1; focus_=0; draw();
                save_frame(captures / (std::string(names[i])+".bmp"));
                SDL_RenderPresent(renderer_.get());
            }
            tools_tab_ = ToolsTab::level_document;
            screen_ = Screen::tools; draw();
            save_frame(captures / "09-level-document-inspector.bmp");
            SDL_RenderPresent(renderer_.get());
        }
        std::cout << "UI smoke checks passed: keyboard, focus, navigation, settings, loss, win, and level inspection.\n";
    }
private:
    Window window_{nullptr, SDL_DestroyWindow};
    Renderer renderer_{nullptr, SDL_DestroyRenderer};
    // Destroy cached engine textures before the host renderer/window.
    std::unique_ptr<engine::SdlRenderer> world_renderer_;
    std::array<engine::SpriteAnimation,2> animations_{
        engine::SpriteAnimation(4,4,{32,32},10),engine::SpriteAnimation(4,4,{32,32},10)};
    std::array<engine::SpriteDraw,2> last_player_draws_{};
    engine::FileResourceManager assets_;
    prototype::SoundBank audio_;
    std::filesystem::path path_;
    engine::EngineSettings settings_{}, draft_{};
    std::array<prototype::PlayableLevelDefinition, 3> campaign_;
    std::size_t campaign_index_{0};
    prototype::DemoLevel game_;
    prototype::OnlinePlay online_;
    bool room_mode_;
    struct SoundFrame {
        struct Gate { bool lever{}, open{}; };
        std::size_t level{};
        std::array<bool,2> grounded{};
        std::vector<Gate> gates;
    };
    std::optional<SoundFrame> previous_sound_frame_;
    std::optional<engine::LevelDocument> inspection_document_;
    std::optional<engine::LevelInspectionSnapshot> inspection_snapshot_;
    engine::InspectionMetrics inspection_metrics_{};
    std::filesystem::path sample_level_path_;
    std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)> font_{nullptr,SDL_DestroyTexture};
    float elapsed_{0};
    std::array<engine::PlayerInput,2> inputs_{};
    Screen screen_{Screen::menu}, previous_{Screen::menu}, tools_return_{Screen::menu};
    enum class ToolsTab { live_entities, level_document };
    ToolsTab tools_tab_{ToolsTab::live_entities};
    bool running_{true}, controls_{false}, capture_{false}, address_active_{false};
    bool level_path_active_{false}, level_status_error_{false};
    float mx_{-1}, my_{-1};
    int focus_{0}, selected_{0};
    std::string notice_, address_{"127.0.0.1"}, level_path_, level_status_, selected_object_id_;
    std::size_t object_page_{0};
    std::vector<Button> buttons_;
    GameplayCamera scene_camera_{};
    engine::Vec2 position(std::size_t i) const { return game_.world().find(game_.player(i).entity)->position; }
    float scene_x(float x) const { return scene_camera_.offset_x + scene_camera_.scale * x; }
    float scene_y(float y) const { return scene_camera_.offset_y + scene_camera_.scale * y; }
    void line(float x1, float y1, float x2, float y2) {
        SDL_RenderLine(renderer_.get(), scene_x(x1), scene_y(y1), scene_x(x2), scene_y(y2));
    }
    void clear_input() { for (auto& p : inputs_) p.clear(); }
    void go(Screen next) {
        clear_input(); screen_ = next; focus_ = 0; controls_ = false;
        address_active_ = false; level_path_active_ = false; SDL_StopTextInput(window_.get());
        if (next == Screen::play) observe_gameplay_sounds(false);
        else previous_sound_frame_.reset();
    }
    void reset_animation() { for(auto& animation:animations_) animation.reset(); }
    void reset_game() { clear_input(); game_.restart(); reset_animation(); }
    void start_campaign() {
        clear_input();
        campaign_index_ = 0;
        reset_animation();
        game_.load(campaign_[campaign_index_]);
    }
    void advance_campaign() {
        if (campaign_index_ + 1 >= campaign_.size()) return;
        clear_input();
        game_.load(campaign_[++campaign_index_]);
        reset_animation();
        go(Screen::play);
    }
    void update_inspection_metrics(Uint64 frame_time_ms) {
        inspection_metrics_.runtime_entity_count = game_.world().size();
        inspection_metrics_.cached_resource_count = assets_.size();
        if (frame_time_ms == 0) {
            inspection_metrics_.fps.reset();
            inspection_metrics_.frame_time_ms.reset();
            return;
        }
        inspection_metrics_.frame_time_ms = static_cast<double>(frame_time_ms);
        inspection_metrics_.fps = 1000.0 / static_cast<double>(frame_time_ms);
    }
    void set_level_status(std::string message, bool is_error = false) {
        level_status_ = std::move(message);
        level_status_error_ = is_error;
        if (is_error) {
            std::cerr << "Level inspector: " << level_status_ << '\n';
        }
    }
    void install_inspection_document(engine::LevelDocument document) {
        auto snapshot = engine::inspect_level_document(document);
        std::string next_selection = selected_object_id_;
        if (snapshot.find_object(next_selection) == nullptr) {
            next_selection.clear();
            for (const auto& object : snapshot.objects) {
                if (snapshot.find_object(object.id) != nullptr) {
                    next_selection = object.id;
                    break;
                }
            }
        }
        inspection_document_ = std::move(document);
        inspection_snapshot_ = std::move(snapshot);
        selected_object_id_ = std::move(next_selection);
        object_page_ = 0;
    }
    void initialize_level_inspector() {
        const char* base_path = SDL_GetBasePath();
        if (base_path == nullptr) {
            set_level_status("Could not locate the packaged inspector sample.", true);
            return;
        }
        sample_level_path_ = std::filesystem::path(base_path) / "assets" / "inspector_sample.level.json";
        level_path_ = sample_level_path_.string();
        try {
            auto document = engine::load_level_file(sample_level_path_);
            install_inspection_document(std::move(document));
            set_level_status("Loaded the bundled sample. Change the path before saving it.");
        } catch (const engine::LevelDocumentError& error) {
            set_level_status(error.what(), true);
        } catch (const engine::LevelFileError& error) {
            set_level_status(error.what(), true);
        }
    }
    void load_inspection_document() {
        try {
            auto document = engine::load_level_file(std::filesystem::path(level_path_));
            install_inspection_document(std::move(document));
            set_level_status("Level document loaded.");
        } catch (const engine::LevelDocumentError& error) {
            set_level_status(error.what(), true);
        } catch (const engine::LevelFileError& error) {
            set_level_status(error.what(), true);
        }
    }
    bool save_path_is_sample() const {
        if (sample_level_path_.empty() || level_path_.empty()) {
            return false;
        }
        std::error_code error;
        const auto sample = std::filesystem::weakly_canonical(sample_level_path_, error);
        if (error) {
            return false;
        }
        const auto target = std::filesystem::weakly_canonical(std::filesystem::path(level_path_), error);
        return !error && target == sample;
    }
    void save_inspection_document() {
        if (!inspection_document_) {
            set_level_status("Load a level document before saving.", true);
            return;
        }
        if (save_path_is_sample()) {
            set_level_status("Change the path before saving; the bundled sample is read-only.", true);
            return;
        }
        try {
            engine::save_level_file(std::filesystem::path(level_path_), *inspection_document_);
            set_level_status("Level document saved.");
        } catch (const engine::LevelDocumentError& error) {
            set_level_status(error.what(), true);
        } catch (const engine::LevelFileError& error) {
            set_level_status(error.what(), true);
        }
    }
    void reload_inspection_document() {
        if (!inspection_document_) {
            set_level_status("Load a level document before reloading.", true);
            return;
        }
        try {
            auto replacement = *inspection_document_;
            engine::reload_level(replacement, std::filesystem::path(level_path_));
            install_inspection_document(std::move(replacement));
            set_level_status("Level document safely reloaded.");
        } catch (const engine::LevelDocumentError& error) {
            set_level_status(error.what(), true);
        } catch (const engine::LevelFileError& error) {
            set_level_status(error.what(), true);
        }
    }
    std::string short_path(const std::string& path) const {
        constexpr std::size_t visible_characters = 78;
        if (path.size() <= visible_characters) {
            return path;
        }
        return "..." + path.substr(path.size() - visible_characters + 3);
    }
    std::string short_text(const std::string& value, std::size_t visible_characters) const {
        if (value.size() <= visible_characters) {
            return value;
        }
        return value.substr(0, visible_characters - 3) + "...";
    }
    std::string metric_value(const std::optional<double>& value) const {
        if (!value) {
            return "N/A";
        }
        std::ostringstream formatted;
        formatted << std::fixed << std::setprecision(1) << *value;
        return formatted.str();
    }
    std::string metric_value(const std::optional<std::size_t>& value) const {
        return value ? std::to_string(*value) : "N/A";
    }
    void exercise_inspector(const auto& check) {
        update_inspection_metrics(16);
        check(inspection_metrics_.runtime_entity_count == game_.world().size(),
              "Populate metrics from the live world");
        check(inspection_metrics_.cached_resource_count == assets_.size(),
              "Inspection reports the cached file resource count");
        check(metric_value(inspection_metrics_.cached_resource_count) == std::to_string(assets_.size()),
              "Format the cached file resource count for display");
        const auto live_entity_count = game_.world().size();
        const auto fire_position = position(0);
        const auto water_position = position(1);
        screen_ = Screen::play;
        key(SDL_SCANCODE_F3, true);
        check(screen_ == Screen::tools, "F3 opens Developer Tools");
        draw();
        click(180, 640);
        check(tools_tab_ == ToolsTab::level_document, "Open Level Document view");
        check(inspection_snapshot_ && inspection_snapshot_->name == "Inspector Sample Room",
              "Load bundled level document");
        check(inspection_snapshot_->objects.size() == 3 && inspection_snapshot_->spawns.size() == 2,
              "Inspect sample document counts");
        draw();
        click(180, 440);
        check(selected_object_id_ == "plate-main", "Select level object by stable ID");

        const auto temporary_path = path_.parent_path() / "inspector-roundtrip.level.json";
        std::filesystem::create_directories(temporary_path.parent_path());
        draw();
        enter_level_path(temporary_path.string());
        draw();
        click(980, 135);
        check(std::filesystem::is_regular_file(temporary_path), "Save inspected document");
        const auto saved = engine::load_level_file(temporary_path);
        check(saved.name == "Inspector Sample Room" && saved.objects.size() == 3,
              "Saved native document fields");

        auto edited = saved;
        edited.name = "Reloaded Inspector Room";
        engine::save_level_file(temporary_path, edited);
        draw();
        click(1150, 135);
        check(inspection_snapshot_ && inspection_snapshot_->name == "Reloaded Inspector Room",
              "Reload native document");
        check(selected_object_id_ == "plate-main", "Preserve selection after successful reload");

        const auto previous_name = inspection_snapshot_->name;
        const auto previous_selection = selected_object_id_;
        {
            std::ofstream invalid_file(temporary_path, std::ios::binary | std::ios::trunc);
            invalid_file << "{invalid";
        }
        draw();
        click(1150, 135);
        check(inspection_snapshot_->name == previous_name && selected_object_id_ == previous_selection,
              "Failed reload preserves document and selection");
        check(level_status_error_, "Failed reload displays an error");

        const auto second_path = path_.parent_path() / "inspector-load.level.json";
        auto second_document = saved;
        second_document.name = "Loaded Inspector Room";
        engine::save_level_file(second_path, second_document);
        draw();
        enter_level_path(second_path.string());
        draw();
        click(840, 135);
        check(inspection_snapshot_->name == "Loaded Inspector Room",
              "Load another native document");
        check(game_.world().size() == live_entity_count &&
              position(0).x == fire_position.x && position(0).y == fire_position.y &&
              position(1).x == water_position.x && position(1).y == water_position.y,
              "Document operations leave the live ECS unchanged");

        key(SDL_SCANCODE_ESCAPE, true);
        check(screen_ == Screen::play, "Escape returns to gameplay");
    }
    void enter_level_path(const std::string& path) {
        click(200, 140);
        SDL_Event select_all{};
        select_all.type = SDL_EVENT_KEY_DOWN;
        select_all.key.scancode = SDL_SCANCODE_A;
        select_all.key.mod = SDL_KMOD_CTRL;
        handle(select_all);
        std::array<char, 32> text_chunk{};
        constexpr std::size_t chunk_size = 31;
        for (std::size_t offset = 0; offset < path.size(); offset += chunk_size) {
            SDL_Event input{};
            input.type = SDL_EVENT_TEXT_INPUT;
            const auto count = std::min(chunk_size, path.size() - offset);
            std::copy_n(path.data() + offset, count, text_chunk.data());
            text_chunk[count] = '\0';
            input.text.text = text_chunk.data();
            handle(input);
        }
    }
    void color(SDL_Color c) { SDL_SetRenderDrawColor(renderer_.get(), c.r,c.g,c.b,c.a); }
    void rect(float x,float y,float w,float h,SDL_Color c) {
        color(c);
        SDL_FRect box{scene_x(x),scene_y(y),scene_camera_.scale*w,scene_camera_.scale*h};
        SDL_RenderFillRect(renderer_.get(), &box);
    }
    void load_font() {
        const auto atlas = assets_.load("font.bmp");
        SDL_IOStream* stream = SDL_IOFromConstMem(atlas->bytes.data(), atlas->bytes.size());
        if (!stream)
            throw std::runtime_error("Cannot open font atlas '" + atlas->source.string() + "': " + SDL_GetError());
        // SDL closes the stream after decoding. The atlas handle keeps its bytes alive until then.
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> source(
            SDL_LoadBMP_IO(stream, true), SDL_DestroySurface);
        if (!source)
            throw std::runtime_error("Cannot decode font atlas '" + atlas->source.string() + "': " + SDL_GetError());
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
        x = scene_x(x);
        y = scene_y(y);
        scale *= scene_camera_.scale;
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
            line(x-dx,y+float(dy),x+dx,y+float(dy));
        }
    }
    void circle(float x,float y,float radius,SDL_Color c) { ellipse(x,y,radius,radius,c); }
    void rounded(float x,float y,float w,float h,float radius,SDL_Color c) {
        radius=std::min(radius,std::min(w,h)/2); color(c);
        for(int row=0;row<int(h);++row) {
            const float dy=row<radius?radius-float(row):float(row)>h-radius?float(row)-(h-radius):0;
            const float inset=dy>0?radius-std::sqrt(std::max(0.0F,radius*radius-dy*dy)):0;
            line(x+inset,y+float(row),x+w-inset,y+float(row));
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
            for(std::size_t i=0;i+1<xs.size();i+=2) line(xs[i],float(y),xs[i+1],float(y));
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
        line(x-4*size,feet-16*size,x,feet-14*size);
        line(x,feet-14*size,x+4*size,feet-16*size);
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
    void sound(Cue cue) { audio_.play(cue, settings_.audio); }

    // Observe the same state in local and network play without modifying the simulation.
    // Start silently on each level or return to play so old events do not replay.
    void observe_gameplay_sounds(bool emit) {
        if (!previous_sound_frame_ || previous_sound_frame_->level != campaign_index_ ||
            previous_sound_frame_->gates.size() != game_.gate_count()) {
            auto& frame = previous_sound_frame_.emplace();
            frame.level = campaign_index_;
            frame.gates.resize(game_.gate_count());
            emit = false;
        }
        auto& last = *previous_sound_frame_;
        for (std::size_t i = 0; i < last.grounded.size(); ++i) {
            const auto& player = game_.player(i);
            if (emit && last.grounded[i] && !player.grounded && player.body.velocity.y < 0)
                sound(i == 0 ? Cue::fire_jump : Cue::water_jump);
            last.grounded[i] = player.grounded;
        }
        bool lever_changed = false, door_opened = false;
        for (std::size_t i = 0; i < last.gates.size(); ++i) {
            const SoundFrame::Gate now{game_.gate_lever_on(i), game_.gate_open(i)};
            lever_changed |= now.lever != last.gates[i].lever;
            door_opened |= now.open && !last.gates[i].open;
            last.gates[i] = now;
        }
        if (emit && lever_changed) sound(Cue::lever);
        if (emit && door_opened) sound(Cue::gate);
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
            settings_=draft_; audio_.apply(settings_.audio); notice_="Settings applied and saved."; sound(Cue::select);
        } catch(const std::exception& error) {
            SDL_SetWindowFullscreen(window_.get(),settings_.window.fullscreen);
            if(!settings_.window.fullscreen) SDL_SetWindowSize(window_.get(),settings_.window.width,settings_.window.height);
            notice_="Could not apply/save settings. Previous settings retained. See terminal.";
            std::cerr << error.what() << '\n';
        }
    }
    void activate(int id) {
        if (id != 7 && id != 19) sound(Cue::select);
        switch(id) {
        case 1: start_campaign(); go(Screen::play); break;
        case 2: go(Screen::lobby); break;
        case 3: previous_=screen_; draft_=settings_; go(Screen::settings); break;
        case 4: running_=false; break;
        case 5: if(online_.active()) online_.request(elemental_coop::ControlKind::resume); else go(Screen::play); break;
        case 6: online_.leave(); go(Screen::menu); break;
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
        case 18: reset_game(); notice_="Active level reloaded."; break;
        case 19: audio_.play(Cue::water_jump, draft_.audio); break;
        case 20: if(online_.active()) online_.request(elemental_coop::ControlKind::pause); else go(Screen::pause); break;
        case 33:
            if(!online_.active()) { reset_game(); go(Screen::play); break; }
            // Online the host restarts the level for both; the joiner returns to play when it sees that.
            online_.request(elemental_coop::ControlKind::restart);
            online_.request(elemental_coop::ControlKind::resume);
            if(online_.role()==prototype::OnlinePlay::Role::host) { clear_input(); reset_animation(); go(Screen::play); }
            break;
        case 40: notice_.clear(); if(!online_.host()) notice_=online_.error(); break;
        case 41: notice_.clear(); if(!online_.join(address_)) notice_=online_.error(); break;
        case 42: start_campaign(); online_.start(); go(Screen::play); break;
        case 43:
            if (const auto code = online_.room_code(); !code.empty())
                notice_ = SDL_SetClipboardText(code.c_str()) ? "Room code copied." : "Could not copy room code.";
            break;
        case 34: advance_campaign(); break;
        case 21:
            tools_tab_ = ToolsTab::level_document;
            level_path_active_ = false;
            SDL_StopTextInput(window_.get());
            break;
        case 22:
            tools_tab_ = ToolsTab::live_entities;
            level_path_active_ = false;
            SDL_StopTextInput(window_.get());
            break;
        case 23:
            if (object_page_ > 0) --object_page_;
            break;
        case 24:
            if (inspection_snapshot_ &&
                (object_page_ + 1) * 3 < inspection_snapshot_->objects.size()) {
                ++object_page_;
            }
            break;
        case 25: load_inspection_document(); break;
        case 26: save_inspection_document(); break;
        case 27: reload_inspection_document(); break;
        case 30:
        case 31:
        case 32: {
            const auto index = object_page_ * 3 + static_cast<std::size_t>(id - 30);
            if (inspection_snapshot_ && index < inspection_snapshot_->objects.size()) {
                const auto& object = inspection_snapshot_->objects[index];
                if (inspection_snapshot_->find_object(object.id) != nullptr) {
                    selected_object_id_ = object.id;
                    level_status_.clear();
                    level_status_error_ = false;
                } else {
                    set_level_status("This object ID is ambiguous and cannot be selected.", true);
                }
            }
            break;
        }
        default: break;
        }
    }
    void click(float x,float y) {
        mx_=x; my_=y;
        if(screen_==Screen::settings && x>=680 && x<=960 && y>=140 && y<=180) { draft_.audio.master_volume=(x-680)/280; return; }
        if(screen_==Screen::lobby && x>=260 && x<1020 && y>=200 && y<=255) {
            address_active_=true; SDL_StartTextInput(window_.get()); return;
        }
        if (screen_ == Screen::tools && tools_tab_ == ToolsTab::level_document &&
            x >= 50 && x <= 760 && y >= 120 && y <= 166) {
            address_active_ = false;
            level_path_active_ = true;
            SDL_StartTextInput(window_.get());
            return;
        }
        for(const auto& b:buttons_) {
            if(b.enabled && x>=b.rect.x && x<=b.rect.x+b.rect.w && y>=b.rect.y && y<=b.rect.y+b.rect.h) {
                const int id=b.id;
                level_path_active_ = false;
                address_active_ = false;
                SDL_StopTextInput(window_.get());
                activate(id);
                break;
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
            // Online the other player keeps playing; this machine just drops its held keys.
            clear_input(); if(screen_==Screen::play && !online_.active()) go(Screen::pause); return;
        }
        if(e.type==SDL_EVENT_MOUSE_MOTION || e.type==SDL_EVENT_MOUSE_BUTTON_DOWN) {
            SDL_ConvertEventToRenderCoordinates(renderer_.get(),&e);
            if(e.type==SDL_EVENT_MOUSE_MOTION) { mx_=e.motion.x; my_=e.motion.y; }
            else if(e.button.button==SDL_BUTTON_LEFT) click(e.button.x,e.button.y);
            return;
        }
        if (e.type == SDL_EVENT_TEXT_INPUT && screen_ == Screen::tools &&
            tools_tab_ == ToolsTab::level_document && level_path_active_) {
            for (const char* p = e.text.text; *p != '\0' && level_path_.size() < 1024; ++p) {
                level_path_ += *p;
            }
            return;
        }
        if(e.type==SDL_EVENT_TEXT_INPUT && screen_==Screen::lobby && address_active_) {
            for(const char* p=e.text.text; *p && address_.size()<40; ++p)
                if((*p>='0' && *p<='9') || (*p>='a' && *p<='z') || (*p>='A' && *p<='Z') ||
                   (!room_mode_ && (*p=='.' || *p==':' || *p=='-'))) address_+=*p;
            return;
        }
        if(e.type!=SDL_EVENT_KEY_DOWN && e.type!=SDL_EVENT_KEY_UP) return;
        const auto scan=e.key.scancode;
        const bool down=e.type==SDL_EVENT_KEY_DOWN;
        if(down && !e.key.repeat) {
            if(scan==SDL_SCANCODE_F12) { capture_=true; return; }
            if(scan==SDL_SCANCODE_F3) { if(screen_==Screen::tools) go(tools_return_); else { tools_return_=screen_;go(Screen::tools); } return; }
            if(scan==SDL_SCANCODE_ESCAPE) {
                // Online, pause is shared: ask the host, and update() follows the shared state.
                if(online_.active() && (screen_==Screen::play || screen_==Screen::pause))
                    online_.request(screen_==Screen::play ? elemental_coop::ControlKind::pause : elemental_coop::ControlKind::resume);
                else if(screen_==Screen::play) go(Screen::pause);
                else if(screen_==Screen::pause) go(Screen::play);
                else if(screen_==Screen::settings) go(previous_);
                else if(screen_==Screen::tools) go(tools_return_);
                else if(screen_==Screen::lobby) { online_.leave(); go(Screen::menu); }
                else go(Screen::menu);
                return;
            }
            if(screen_!=Screen::play) {
                if (room_mode_ && address_active_ && (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0 &&
                    scan == SDL_SCANCODE_V) {
                    std::unique_ptr<char, decltype(&SDL_free)> clipboard(SDL_GetClipboardText(), SDL_free);
                    if (clipboard) {
                        const std::string_view pasted = clipboard.get();
                        if (pasted.size() <= 40 && pasted.find_first_not_of(
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") == std::string_view::npos)
                            address_ = pasted;
                    }
                    return;
                }
                if (screen_ == Screen::tools && tools_tab_ == ToolsTab::level_document &&
                    level_path_active_ && (e.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0 &&
                    scan == SDL_SCANCODE_A) {
                    level_path_.clear();
                    return;
                }
                if (screen_ == Screen::tools && tools_tab_ == ToolsTab::level_document &&
                    level_path_active_ && scan == SDL_SCANCODE_BACKSPACE && !level_path_.empty()) {
                    auto index = level_path_.size() - 1;
                    while (index > 0 &&
                           (static_cast<unsigned char>(level_path_[index]) & 0xC0U) == 0x80U) {
                        --index;
                    }
                    level_path_.erase(index);
                    return;
                }
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
        if(down && scan==SDL_SCANCODE_E) { if(online_.active()) online_.interact(); else game_.interact(); return; }
        const std::array<std::array<SDL_Scancode,3>,2> keys{{{SDL_SCANCODE_A,SDL_SCANCODE_D,SDL_SCANCODE_W},
            {SDL_SCANCODE_LEFT,SDL_SCANCODE_RIGHT,SDL_SCANCODE_UP}}};
        for(std::size_t p=0;p<2;++p) for(int a=0;a<3;++a) if(scan==keys[p][a]) {
            const auto action=static_cast<engine::PlayerAction>(a);
            if(down) inputs_[p].press(action); else inputs_[p].release(action);
        }
    }
    // Online: OnlinePlay runs the level (host) or copies the host's level (joiner), every frame on every
    // screen. Screens follow the shared state. Returns false when nothing below should run this frame.
    bool update_online(float dt) {
        online_.update({inputs_[0].consume(),inputs_[1].consume()},dt);
        if(online_.connection_lost()) {
            notice_=online_.status();
            online_.leave();
            go(screen_ == Screen::lobby ? Screen::lobby : Screen::menu);
            return false;
        }
        if(!online_.playing()) return false; // still in the lobby
        if(screen_==Screen::lobby) go(Screen::play);
        if(screen_==Screen::play && online_.paused()) go(Screen::pause);
        else if(screen_==Screen::pause && !online_.paused()) go(Screen::play);
        // The host retried or moved on: the joiner leaves the result screen too.
        if((screen_==Screen::lost || screen_==Screen::won) && game_.state()==elemental_coop::RoundState::playing) {
            reset_animation(); go(Screen::play);
        }
        return screen_==Screen::play;
    }
    void update(float dt) {
        audio_.update_music();
        elapsed_+=dt;
        if(online_.active() && !update_online(dt)) return;
        if(screen_!=Screen::play || dt<=0) return;
        if(!online_.active()) game_.tick({inputs_[0].consume(),inputs_[1].consume()},dt);
        for(std::size_t i=0;i<2;++i) {
            const auto& state=game_.player(i);
            if(std::abs(state.body.velocity.x)>0.01F) {
                animations_[i].advance(dt);
            } else animations_[i].reset();
        }
        if(game_.state()==elemental_coop::RoundState::lost) { go(Screen::lost); sound(Cue::defeat); }
        else if(game_.state()==elemental_coop::RoundState::won) { go(Screen::won); sound(Cue::complete); }
        else observe_gameplay_sounds(true);
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
        std::vector<engine::SpriteDraw> tiles;
        tiles.reserve(static_cast<std::size_t>(count));
        for(int i=0;i<count;++i) {
            const float x=r.x+float(i)*tile;
            engine::SpriteDraw draw;
            draw.position={x,r.y};draw.size={tile,r.h};draw.asset_key="tiles";
            draw.source=engine::RenderRect{0,0,32,32};tiles.push_back(draw);
        }
        world_renderer_->submit(tiles);
    }
    void portal(float x, float surface_y, bool fire) {
        const auto c = fire ? orange : cyan;
        const float top = surface_y - 104.0F;
        rounded(x - 40, top, 80, 104, 8, {c.r, c.g, c.b, 30});
        rounded(x - 34, top + 3, 68, 101, 6, c);
        rect(x - 25, top + 12, 50, 92, {9, 42, 65, 255});
        if (game_.all_gates_open())
            rect(x - 24, top + 13, 48, 90, {c.r, c.g, c.b, 70});
        text(x - 50, surface_y - 137.0F, fire ? "FIRE EXIT" : "WATER EXIT", 1.5F, c);
    }

    void gameplay() {
        const auto& definition = game_.definition();
        scene_camera_ = {};
        text(35, 25, definition.name, 3.0F);
        text(420, 32, game_.all_gates_open()
             ? "All gates open - reach your matching exits"
             : "Coordinate the linked levers and pressure plates", 1.5F, white);
        scene_camera_ = gameplay_camera(definition.name);
        engine::Camera camera;
        camera.viewport={width,height};camera.zoom=scene_camera_.scale;
        camera.position={-scene_camera_.offset_x/camera.zoom,-scene_camera_.offset_y/camera.zoom};
        world_renderer_->set_camera(camera);
        for (const auto& platform : definition.platforms) stone(platform);
        for (const auto& pool : definition.hazards) {
            const auto c = pool.kind == elemental_coop::Hazard::lava ? SDL_Color{255, 125, 0, 255}
                : pool.kind == elemental_coop::Hazard::water ? cyan : SDL_Color{87, 235, 11, 255};
            gradient(pool.box.x, pool.box.y, pool.box.w, pool.box.h,
                     c, {c.r, Uint8(c.g / 2), Uint8(c.b / 2), 255});
            color(c);
            for (int j = 0; j < static_cast<int>(pool.box.w); ++j) {
                const float y = pool.box.y + std::sin(static_cast<float>(j) * 0.09F + elapsed_ * 3) * 3;
                line(pool.box.x + j, y, pool.box.x + j, pool.box.y + 5);
            }
            for (int j = 0; j < 5; ++j)
                circle(pool.box.x + 12 + static_cast<float>(j) * 26,
                       pool.box.y - 5 - std::sin(elapsed_ * 2 + j) * 5, 2, c);
            const std::string label = pool.kind == elemental_coop::Hazard::lava ? "LAVA"
                : pool.kind == elemental_coop::Hazard::water ? "WATER" : "TOXIC";
            const float label_scale=std::min(1.6F,std::max(0.6F,(pool.box.w-8)*1.6F/text_width(label,1.6F)));
            text(pool.box.x + (pool.box.w - text_width(label, label_scale)) / 2,
                 655, label, label_scale, c);
        }

        for (std::size_t i = 0; i < definition.exits.size(); ++i) {
            const auto& exit = definition.exits[i];
            portal(exit.area.x + exit.area.w / 2.0F,
                   exit.area.y + exit.area.h, i == 0);
        }

        for (std::size_t i = 0; i < definition.gates.size(); ++i) {
            const auto& gate = definition.gates[i];
            const auto& door = gate.door;
            const bool is_open = game_.gate_open(i);
            rounded(door.x - 5, door.y - 5, door.w + 10, door.h + 5,
                    4, {112, 141, 171, 255});
            rect(door.x, door.y, door.w, door.h, {25, 49, 77, 255});
            if (!is_open) {
                gradient(door.x + 6, door.y + 6, door.w - 12, door.h - 6,
                         {108, 128, 151, 255}, {58, 78, 104, 255});
                circle(door.x + door.w / 2, door.y + door.h / 2, 10, navy);
            } else {
                text(door.x + 5, door.y + door.h / 2, "OPEN", 1.2F, cyan);
            }
            const auto gate_label = definition.gates.size() == 1
                ? std::string("DOOR") : "GATE " + std::string(1, static_cast<char>('A' + i));
            text(door.x - 5, door.y - 29, gate_label, 1.3F, white);

            const auto& lever = gate.lever;
            const bool lever_on = game_.gate_lever_on(i);
            const auto lever_color = lever_on ? green : orange;
            const float lever_x = lever.x + lever.w / 2.0F;
            const float lever_bottom = lever.y + lever.h;
            stone({lever.x, lever_bottom - 12, lever.w, 12});
            rect(lever_x - 3, lever.y, 6, lever.h - 12, {159, 182, 204, 255});
            line(lever_x, lever_bottom - 32,
                 lever_on ? lever_x + 14 : lever_x - 14, lever_bottom - 54);
            rounded(lever_on ? lever_x + 5 : lever_x - 22,
                    lever_bottom - 61, 18, 10, 4, lever_color);
            text(lever.x - 12, lever_bottom + 12, "SWITCH [E]", 1.0F, lever_color);

            const auto& plate = gate.plate;
            const bool plate_occupied = game_.gate_plate_occupied(i);
            stone({plate.x - 14, plate.y - 6, plate.w + 28, 10});
            rounded(plate.x - 8, plate_occupied ? plate.y - 6 : plate.y - 11,
                    plate.w + 16, 7, 3, plate_occupied ? green : cyan);
            text(plate.x - 18, plate.y + 15, "PRESSURE PLATE", 0.9F,
                 plate_occupied ? green : cyan);
        }

        // Acid slimes use the pool's exact green, with animated feet and hostile eyes.
        // Their animation clock belongs to the runtime, so menus and pause freeze it.
        for (std::size_t i = 0; i < game_.enemies().size(); ++i) {
            const auto& enemy = game_.enemies()[i];
            const auto p = game_.enemy_position(i);
            const float bob = std::sin(enemy.animation_time * 12) * 2;
            const SDL_Color acid{87, 235, 11, 255};
            rounded(p.x - 16, p.y - 28 + bob, 32, 26 - bob, 10, {31, 102, 6, 255});
            rounded(p.x - 15, p.y - 32 + bob, 30, 27 - bob, 10, acid);
            for (int foot = 0; foot < 3; ++foot)
                circle(p.x - 10 + foot * 10, p.y - 3,
                       4 + std::sin(enemy.animation_time * 12 + foot) * 1.2F, acid);
            circle(p.x - 7, p.y - 25 + bob, 3, {185, 255, 119, 255});
            const float look = enemy.facing_left ? -2.0F : 2.0F;
            for (float eye : {-7.0F, 7.0F}) {
                rounded(p.x + eye - 4, p.y - 21 + bob, 8, 8, 2, white);
                rect(p.x + eye + look - 2, p.y - 20 + bob, 4, 6, navy);
            }
            rect(p.x - 5, p.y - 9 + bob, 10, 3, navy);
        }

        for (std::size_t i = 0; i < elemental_coop::player_count; ++i) {
            const auto p = position(i);
            engine::SpriteDraw sprite;
            sprite.entity=game_.player(i).entity;
            sprite.position={p.x-27.2F,p.y-54.4F};sprite.size={54.4F,54.4F};
            sprite.asset_key="characters";sprite.layer=10;sprite.flip_x=game_.player_facing_left(i);
            sprite.source=animations_[i].source();
            if(!game_.player(i).grounded) sprite.source->x=64; // airborne pose
            sprite.source->y+=static_cast<float>(i)*32;
            last_player_draws_[i]=sprite;
            world_renderer_->submit(std::span(&sprite,1));
            if (game_.player(i).at_exit) circle(p.x, p.y - 72, 4, green);
        }
        scene_camera_ = {};
        text(420, 103, "ACID ENEMIES: " + std::to_string(game_.enemies().size()) + " - jump over them", 1.1F,
             {87, 235, 11, 255});
        text(30, 79, "FIRE  A / D / W     WATER  ARROWS     E  LEVER", 1.1F, muted);
        text(420, 79, game_.all_gates_open()
             ? "Unlocked - both matching exits are required"
             : definition.gates.size() == 1
                 ? "Use different players for the lever and pressure plate"
                 : "Open Gate A, then coordinate Gate B's two controls",
             1.0F, cyan);
    }
    void draw_level_document_view() {
        center(35, "LEVEL DOCUMENT INSPECTOR", 3.2F);
        text(50, 95, "NATIVE JSON PATH  (CTRL/CMD+A TO REPLACE)", 1.1F, muted);
        rounded(50, 120, 710, 46, 7, level_path_active_ ? cyan : blue);
        rounded(54, 124, 702, 38, 5, navy);
        text(64, 135, short_path(level_path_), 1.2F, white);
        button(790, 112, 120, "LOAD", 25, !level_path_.empty());
        button(920, 112, 120, "SAVE", 26, inspection_document_.has_value() && !save_path_is_sample());
        button(1050, 112, 175, "RELOAD", 27, inspection_document_.has_value() && !level_path_.empty());

        if (inspection_snapshot_) {
            const auto& snapshot = *inspection_snapshot_;
            const std::string summary =
                "NAME: " + short_text(snapshot.name, 32) +
                "  BOUNDS: (" + std::to_string(snapshot.bounds.minimum.x) + "," +
                std::to_string(snapshot.bounds.minimum.y) + ")-(" +
                std::to_string(snapshot.bounds.maximum.x) + "," +
                std::to_string(snapshot.bounds.maximum.y) + ")" +
                "  SPAWNS: " + std::to_string(snapshot.spawns.size()) +
                "  OBJECTS: " + std::to_string(snapshot.objects.size());
            text(50, 178, short_text(summary, 145), 1.15F, cyan);
        } else {
            text(50, 178, "No level document loaded.", 1.3F, muted);
        }

        text(50, 202,
             "FPS: " + metric_value(inspection_metrics_.fps) +
             "  FRAME: " + metric_value(inspection_metrics_.frame_time_ms) + " ms" +
             "  LIVE WORLD ENTITIES: " + metric_value(inspection_metrics_.runtime_entity_count) +
             "  CACHED FILE RESOURCES: " + metric_value(inspection_metrics_.cached_resource_count),
             1.1F, muted);

        rounded(50, 230, 450, 360, 10, panel);
        rounded(530, 230, 700, 360, 10, panel);
        text(75, 248, "LEVEL-DOCUMENT OBJECTS (NOT ECS ENTITIES)", 1.35F, cyan);
        text(550, 248, "SELECTED LEVEL OBJECT", 1.5F, cyan);
        if (inspection_snapshot_) {
            constexpr std::size_t objects_per_page = 3;
            const auto& objects = inspection_snapshot_->objects;
            const auto page_count = std::max<std::size_t>(
                1, (objects.size() + objects_per_page - 1) / objects_per_page);
            object_page_ = std::min(object_page_, page_count - 1);
            const auto first = object_page_ * objects_per_page;
            for (std::size_t row = 0; row < objects_per_page && first + row < objects.size(); ++row) {
                const auto& object = objects[first + row];
                const auto label = (object.id == selected_object_id_ ? "> " : "  ") +
                                   short_text(object.id + " [" + object.type + "]", 38);
                button(70, 280 + static_cast<float>(row) * 65, 410, label,
                       30 + static_cast<int>(row));
            }
            button(70, 500, 140, "PREVIOUS", 23, object_page_ > 0);
            text(215, 520, "PAGE " + std::to_string(object_page_ + 1) + "/" +
                 std::to_string(page_count), 1.0F, muted);
            button(350, 500, 130, "NEXT", 24, object_page_ + 1 < page_count);

            const auto* object = inspection_snapshot_->find_object(selected_object_id_);
            if (object == nullptr) {
                text(550, 290, "Select an object with a unique stable ID.", 1.25F, muted);
            } else {
                text(550, 285, "ID: " + short_text(object->id, 78), 1.25F, white);
                text(550, 315, "Type: " + short_text(object->type, 78), 1.25F, white);
                text(550, 345, "Position: " + std::to_string(object->position.x) + ", " +
                     std::to_string(object->position.y), 1.25F, white);
                text(550, 375, "Dimensions: " + std::to_string(object->size.x) + " x " +
                     std::to_string(object->size.y), 1.25F, white);
                text(550, 405, "Prefab: " + short_text(object->prefab.empty() ? "N/A" : object->prefab, 78),
                     1.25F, white);
                text(550, 435, "Parent ID: " + short_text(object->parent_id.empty() ? "N/A" : object->parent_id, 78),
                     1.25F, white);
                text(550, 465, "Linked ID: " + short_text(object->linked_id.empty() ? "N/A" : object->linked_id, 78),
                     1.25F, white);
            }

            text(550, 500, "VALIDATION DIAGNOSTICS", 1.15F, cyan);
            if (inspection_snapshot_->validation_errors.empty()) {
                text(550, 528, "No validation errors.", 1.1F, green);
            } else {
                const auto visible_errors = std::min<std::size_t>(
                    2, inspection_snapshot_->validation_errors.size());
                for (std::size_t i = 0; i < visible_errors; ++i) {
                    text(550, 526 + static_cast<float>(i) * 22,
                         short_text(inspection_snapshot_->validation_errors[i], 100), 0.95F, orange);
                }
                if (inspection_snapshot_->validation_errors.size() > visible_errors) {
                    text(550, 570, "Additional validation errors are available in the inspection snapshot.",
                         0.9F, orange);
                }
            }
        } else {
            text(75, 295, "Load the bundled sample or enter a native level JSON path.", 1.1F, muted);
            text(550, 290, "No selected level object.", 1.25F, muted);
        }

        if (!level_status_.empty()) {
            text(50, 592, short_text(level_status_, 150), 1.0F,
                 level_status_error_ ? orange : green);
        }
        button(50, 610, 300, "LIVE ENTITIES", 22);
        button(440, 610, 400, "BACK", 16);
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
            text(190,215,room_mode_ ? "Code" : "Server",2);
            rect(310,200,760,55,navy);
            text(330,219,room_mode_ && address_.empty() && !address_active_ ? "Enter room code" : address_,2);
        {
            const bool hosting=online_.role()==prototype::OnlinePlay::Role::host;
            const bool joining=online_.role()==prototype::OnlinePlay::Role::joiner;
            text(190,300,hosting?"Player 1 (fire): You":joining?"Player 1 (fire): Host":"Player 1 (fire): Not connected",2,
                 hosting||online_.connected()?green:muted);
            text(190,350,joining?"Player 2 (water): You":online_.connected()?"Player 2 (water): Connected":"Player 2 (water): Not connected",2,
                 joining||online_.connected()?green:muted);
            button(190,420,270,room_mode_ ? "CREATE ROOM" : "HOST SESSION",40,!online_.active());
            button(505,420,270,room_mode_ ? "JOIN ROOM" : "JOIN SESSION",41,!online_.active());
            button(820,420,270,"START GAME",42,hosting&&online_.connected());
            center(505,online_.active() ? online_.status() :
                       room_mode_ ? "Create a room and share the code, or enter a friend's code." :
                                    "Host a session, or type the host's address and join.",2,muted);
            if (room_mode_ && hosting && !online_.connected() && !online_.room_code().empty())
                button(190,535,270,"COPY CODE",43);
            button(440,600,400,"BACK",6); break;
        }
        case Screen::play: gameplay(); button(1090,20,155,"PAUSE",20); break;
        case Screen::pause:
            gameplay(); rect(0,0,1280,690,{0,12,29,155}); rounded(389,169,502,456,14,blue); rounded(391,171,498,452,13,panel); center(210,"PAUSED",4);
            button(440,280,400,"RESUME",5); button(440,355,400,"RESTART",33);
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
            button(160,465,400,"VIEW CONTROLS",13);
            button(670,465,400,"PREVIEW SOUND",19,audio_.available(Cue::water_jump) && draft_.audio.sound_effects && draft_.audio.master_volume>0.0F);
            if(controls_) center(550,"Fire: A/D/W | Water: arrows | E: lever | Esc: pause",1.3F,cyan);
            else center(550,audio_.available(Cue::water_jump)
                 ? "Preview uses these settings. Apply saves them; Back discards changes."
                 : "Sound preview unavailable. Apply saves other settings.",1.2F,muted);
            button(270,620,330,"APPLY",7); button(680,620,330,"BACK",8); break;
        }
        case Screen::lost:
        case Screen::won: {
            const bool won=screen_==Screen::won;
            const bool campaign_complete = won && campaign_index_ + 1 == campaign_.size();
            center(70,campaign_complete?"CAMPAIGN COMPLETE!":won?"LEVEL COMPLETE!":"GAME OVER",4);
            center(145,campaign_complete?"All three levels completed cooperatively":
                   won?"Both players reached their exits":"A character was defeated",2,muted);
            character(470,420,true,3); character(810,420,false,3);
            const bool joining=online_.role()==prototype::OnlinePlay::Role::joiner; // the host picks the level
            if(won && !campaign_complete) {
                button(100,505,340,"NEXT LEVEL",34,!joining);
                button(470,505,340,"REPLAY LEVEL",33);
                button(840,505,340,"MAIN MENU",6);
                center(610,game_.definition().name,2,muted);
            } else if(campaign_complete) {
                button(260,505,350,"REPLAY CAMPAIGN",1,!joining);
                button(670,505,350,"MAIN MENU",6);
            } else {
                button(260,505,350,"RETRY",33);
                button(670,505,350,"MAIN MENU",6);
            }
            break;
        }
        case Screen::tools: {
            if (tools_tab_ == ToolsTab::level_document) {
                draw_level_document_view();
            } else {
                center(50,"DEVELOPER TOOLS",4);
                text(60,120,"LEVEL: " + game_.definition().name,2,cyan);
                button(800,105,420,"RELOAD DEMO LEVEL",18);
                rounded(50,190,450,390,10,panel); rounded(530,190,700,390,10,panel);
                text(80,220,"LIVE PLAYER ENTITIES",2);
                button(80,275,380,selected_==0?"> PLAYER FIRE":"> PLAYER WATER",17);
                text(80,370,"2 controllable players",2,muted);
                text(80,410,"Click to change selection",1.5F,muted);
                const auto p=position(static_cast<std::size_t>(selected_));
                const auto& state=game_.player(static_cast<std::size_t>(selected_));
                text(560,220,"SELECTED LIVE ECS ENTITY",1.65F);
                text(560,285,"Position: "+std::to_string(static_cast<int>(p.x))+", "+std::to_string(static_cast<int>(p.y)),2);
                text(560,335,"Velocity: "+std::to_string(static_cast<int>(state.body.velocity.x))+", "+std::to_string(static_cast<int>(state.body.velocity.y)),2);
                text(560,385,state.grounded?"Grounded: yes":"Grounded: no",2);
                text(560,435,"World entities: "+std::to_string(game_.world().size()),2);
                text(560,490,"Simulation frozen while inspecting",1.5F,muted);
                text(560,535,
                     "FPS: " + metric_value(inspection_metrics_.fps) +
                     "  FRAME: " + metric_value(inspection_metrics_.frame_time_ms) + " ms" +
                     "  CACHED FILE RESOURCES: " + metric_value(inspection_metrics_.cached_resource_count),
                     1.0F, muted);
                button(50,610,300,"LEVEL DOCUMENT",21);
                button(440,610,400,"BACK",16);
            }
            break;
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
        // The UI smoke test can run without socket access or a free network port.
        const char* relay = std::getenv("ELEMENTAL_COOP_RELAY");
        if (!relay) relay = GROUP2_RELAY_ADDRESS;
        const bool room_mode = !smoke && *relay;
        auto session = smoke ? engine::net::make_loopback_pair().first :
            room_mode ? engine::net::make_room_session(relay) : engine::net::make_enet_session();
        App app(path, std::move(session), room_mode);
        if(smoke) { app.smoke_test(argc>2?std::filesystem::path(argv[2]):std::filesystem::path{}); std::filesystem::remove_all(path.parent_path()); return 0; }
        return app.run();
    } catch(const std::exception& e) { std::cerr<<"UI prototype: "<<e.what()<<'\n'; return 1; }
}
