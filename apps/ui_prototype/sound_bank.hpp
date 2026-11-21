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

public:
    void open(engine::FileResourceManager& files) {
        device_.id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!device_.id) {
            std::cerr << "Sound output unavailable: " << SDL_GetError() << '\n';
            return;
        }
        for (std::size_t i = 0; i < names.size(); ++i) {
            const std::string name = std::string("audio/") + names[i] + ".wav";
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
                voices_[i] = std::move(voice);
            } catch (const std::exception& e) {
                std::cerr << "Sound unavailable (" << name << "): " << e.what() << '\n';
            }
        }
    }

    bool available(Cue cue) const { return static_cast<bool>(voices_[static_cast<std::size_t>(cue)].stream); }
    SDL_AudioDeviceID device() const { return device_.id; }
    int queued(Cue cue) const {
        const auto& voice = voices_[static_cast<std::size_t>(cue)];
        return voice.stream ? SDL_GetAudioStreamQueued(voice.stream.get()) : 0;
    }
    void apply(const engine::AudioSettings& settings) {
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
