#pragma once
#include <SDL3/SDL.h>
#include "engine/configuration.hpp"
#include "engine/resource_files.hpp"
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace prototype {
enum class Cue : std::size_t { select, fire_jump, water_jump, lever, gate, defeat, complete, count };

// The shared cache owns file bytes. Each effect has a decoded clip and its own
// SDL stream so simultaneous jumps can mix without building up a playback queue.
class SoundBank {
    struct Device {
        SDL_AudioDeviceID id{};
        ~Device() { if (id) SDL_CloseAudioDevice(id); }
    } device_; // declared first: all streams are destroyed before the device
    struct Voice {
        std::unique_ptr<SDL_AudioStream, decltype(&SDL_DestroyAudioStream)> stream{nullptr, SDL_DestroyAudioStream};
        std::unique_ptr<Uint8, decltype(&SDL_free)> pcm{nullptr, SDL_free};
        int size{};
    };
    static constexpr std::array names{"select", "fire_jump", "water_jump", "lever", "gate", "defeat", "complete"};
    static_assert(names.size() == static_cast<std::size_t>(Cue::count));
    std::array<Voice, names.size()> voices_{};

    Voice music_;
    bool music_enabled_{};
    bool music_muted_{};

    void apply_music_volume(float master_volume) {
        if (!music_.stream) return;
        const float gain = music_muted_ ? 0.0F : 0.55F * master_volume;
        SDL_SetAudioStreamGain(music_.stream.get(), gain);
        music_enabled_ = gain > 0.0F;
        if (!music_enabled_) SDL_ClearAudioStream(music_.stream.get());
        update_music();
    }

    Voice load(engine::FileResourceManager& files, const std::string& name) {
        try {
            const auto asset = files.load(name);
            SDL_AudioSpec format{};
            Uint8* pcm = nullptr;
            Uint32 size = 0;
            auto* io = SDL_IOFromConstMem(asset->bytes.data(), asset->bytes.size());
            if (!io || !SDL_LoadWAV_IO(io, true, &format, &pcm, &size))
                throw std::runtime_error(SDL_GetError());
            Voice voice;
            voice.pcm.reset(pcm);
            if (!size || size > static_cast<Uint32>(std::numeric_limits<int>::max()))
                throw std::runtime_error("empty or oversized recording");
            voice.size = static_cast<int>(size);
            voice.stream.reset(SDL_CreateAudioStream(&format, &format));
            if (!voice.stream || !SDL_BindAudioStream(device_.id, voice.stream.get()))
                throw std::runtime_error(SDL_GetError());
            return voice;
        } catch (const std::exception& e) {
            std::cerr << "Sound unavailable (" << name << "): " << e.what() << '\n';
            return {};
        }
    }

public:
    void open(engine::FileResourceManager& files) {
        device_.id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!device_.id) {
            std::cerr << "Sound output unavailable: " << SDL_GetError() << '\n';
            return;
        }
        for (std::size_t i = 0; i < names.size(); ++i)
            voices_[i] = load(files, std::string("audio/") + names[i] + ".wav");
        music_ = load(files, "audio/background.wav");
    }

    int music_queued() const {
        return music_.stream ? SDL_GetAudioStreamQueued(music_.stream.get()) : 0;
    }
    bool music_available() const { return static_cast<bool>(music_.stream); }
    bool music_muted() const { return music_muted_; }
    void toggle_music(float master_volume) {
        if (!music_available()) return;
        music_muted_ = !music_muted_;
        apply_music_volume(master_volume);
    }

    void update_music() {
        if (!music_.stream || !music_enabled_) return;
        const int queued = music_queued();
        // Append the next loop early. Flushing between loops would end resampling
        // at each boundary and can create an audible seam.
        if (queued >= 0 && queued <= music_.size / 4 &&
            !SDL_PutAudioStreamData(music_.stream.get(), music_.pcm.get(), music_.size)) {
            std::cerr << "Background music unavailable: " << SDL_GetError() << '\n';
            music_enabled_ = false;
        }
    }

    bool available(Cue cue) const { return static_cast<bool>(voices_[static_cast<std::size_t>(cue)].stream); }
    SDL_AudioDeviceID device() const { return device_.id; }
    int queued(Cue cue) const {
        const auto& voice = voices_[static_cast<std::size_t>(cue)];
        return voice.stream ? SDL_GetAudioStreamQueued(voice.stream.get()) : 0;
    }
    void apply(const engine::AudioSettings& settings) {
        apply_music_volume(settings.master_volume);
        const float gain = settings.sound_effects ? settings.master_volume : 0.0F;
        for (auto& voice : voices_) if (voice.stream) {
            SDL_SetAudioStreamGain(voice.stream.get(), gain);
            if (gain == 0.0F) SDL_ClearAudioStream(voice.stream.get());
        }
    }
    bool play(Cue cue, const engine::AudioSettings& settings) {
        auto& voice = voices_[static_cast<std::size_t>(cue)];
        if (!voice.stream || !settings.sound_effects || settings.master_volume <= 0.0F) return false;
        return SDL_ClearAudioStream(voice.stream.get()) &&
               SDL_SetAudioStreamGain(voice.stream.get(), settings.master_volume) &&
               SDL_PutAudioStreamData(voice.stream.get(), voice.pcm.get(), voice.size) &&
               SDL_FlushAudioStream(voice.stream.get());
    }
};
} // namespace prototype
