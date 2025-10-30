#include "audio/AudioEngine.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
#include <alsa/asoundlib.h>
#endif

namespace {
struct MelodyNote {
    float frequency;
    float duration;
    float amplitude;
};

#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
constexpr unsigned kSampleRate = 48000;
constexpr unsigned kChannels = 2;
constexpr std::size_t kFramesPerBuffer = 512;
#endif
} // namespace

#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
struct AudioEngine::LinuxBackend {
    struct SequenceState {
        std::vector<MelodyNote> notes;
        std::size_t index{0};
        float timer{0.0f};
        float phase{0.0f};
    };

    snd_pcm_t* handle{nullptr};
    std::vector<std::int16_t> mixBuffer;
    SequenceState lead;
    SequenceState bass;
    SequenceState harmony;

    bool initialize() {
        int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
        if (err < 0) {
            std::cerr << "AudioEngine: Failed to open ALSA device: " << snd_strerror(err) << "\n";
            return false;
        }

        err = snd_pcm_set_params(handle,
                                 SND_PCM_FORMAT_S16_LE,
                                 SND_PCM_ACCESS_RW_INTERLEAVED,
                                 kChannels,
                                 kSampleRate,
                                 1,
                                 40000);
        if (err < 0) {
            std::cerr << "AudioEngine: Failed to configure ALSA device: " << snd_strerror(err) << "\n";
            snd_pcm_close(handle);
            handle = nullptr;
            return false;
        }

        mixBuffer.resize(kFramesPerBuffer * kChannels);
        buildSequences();
        return true;
    }

    void buildSequences() {
        // A simple arpeggiated chord progression reminiscent of retro RPGs.
        lead.notes = {
            {392.0f, 0.35f, 0.35f}, // G4
            {440.0f, 0.25f, 0.35f}, // A4
            {523.25f, 0.35f, 0.35f}, // C5
            {587.33f, 0.45f, 0.35f}, // D5
            {523.25f, 0.25f, 0.35f},
            {659.26f, 0.35f, 0.35f}, // E5
            {698.46f, 0.45f, 0.35f}, // F5
            {783.99f, 0.55f, 0.35f}, // G5
        };

        bass.notes = {
            {98.0f, 0.5f, 0.25f}, // G2
            {82.41f, 0.5f, 0.25f}, // E2
            {87.31f, 0.5f, 0.25f}, // F2
            {92.50f, 0.5f, 0.25f}, // F#2
        };

        harmony.notes = {
            {261.63f, 0.75f, 0.2f}, // C4
            {329.63f, 0.75f, 0.2f}, // E4
            {392.0f, 0.75f, 0.2f}, // G4
            {440.0f, 0.75f, 0.2f}, // A4
        };
    }

    void shutdown() {
        if (handle) {
            snd_pcm_drain(handle);
            snd_pcm_close(handle);
            handle = nullptr;
        }
    }
};
#endif // defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    stop();
    shutdown();
}

bool AudioEngine::initialize() {
#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    if (initialized_) {
        return backend_ != nullptr;
    }

    backend_ = new LinuxBackend();
    if (!backend_->initialize()) {
        delete backend_;
        backend_ = nullptr;
        return false;
    }

    initialized_ = true;
    return true;
#else
    if (!initialized_) {
        std::cerr << "AudioEngine: ALSA support disabled, skipping background music initialization.\n";
        initialized_ = true;
    }
    return false;
#endif
}

void AudioEngine::start() {
    if (!initialized_ || running_) {
        return;
    }
#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    if (!backend_) {
        return;
    }
#endif
    running_ = true;
#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    worker_ = std::thread(&AudioEngine::audioThread, this);
#endif
}

void AudioEngine::stop() {
    if (!running_) {
        return;
    }
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void AudioEngine::shutdown() {
#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    if (backend_) {
        backend_->shutdown();
        delete backend_;
        backend_ = nullptr;
    }
#endif
    initialized_ = false;
}

void AudioEngine::audioThread() {
#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    if (!backend_ || !backend_->handle) {
        return;
    }

    auto sampleSequence = [](AudioEngine::LinuxBackend::SequenceState& seq, float dt, float (*wave)(float)) {
        if (seq.notes.empty()) {
            return 0.0f;
        }

        seq.timer += dt;
        const MelodyNote& note = seq.notes[seq.index];
        if (seq.timer >= note.duration) {
            seq.timer -= note.duration;
            seq.index = (seq.index + 1) % seq.notes.size();
        }

        seq.phase += dt * note.frequency * 2.0f * static_cast<float>(M_PI);
        if (seq.phase > 2.0f * static_cast<float>(M_PI)) {
            seq.phase = std::fmod(seq.phase, 2.0f * static_cast<float>(M_PI));
        }

        return note.amplitude * wave(seq.phase);
    };

    auto sine = [](float phase) { return std::sin(phase); };
    auto triangle = [](float phase) {
        float value = phase / static_cast<float>(M_PI);
        value = value - std::floor(value);
        return 4.0f * std::fabs(value - 0.5f) - 1.0f;
    };

    const float dt = 1.0f / static_cast<float>(kSampleRate);

    while (running_) {
        for (std::size_t frame = 0; frame < kFramesPerBuffer; ++frame) {
            float sample = 0.0f;
            sample += sampleSequence(backend_->lead, dt, sine);
            sample += sampleSequence(backend_->bass, dt, triangle) * 0.6f;
            sample += sampleSequence(backend_->harmony, dt, sine) * 0.8f;

            // Subtle low-frequency oscillation for atmosphere.
            static float lfoPhase = 0.0f;
            lfoPhase += dt * 0.5f * 2.0f * static_cast<float>(M_PI);
            if (lfoPhase > 2.0f * static_cast<float>(M_PI)) {
                lfoPhase -= 2.0f * static_cast<float>(M_PI);
            }
            float lfo = 0.15f * std::sin(lfoPhase);
            sample *= (0.85f + lfo);

            sample = std::clamp(sample, -0.95f, 0.95f);
            std::int16_t pcm = static_cast<std::int16_t>(sample * 32767.0f);
            backend_->mixBuffer[frame * 2 + 0] = pcm;
            backend_->mixBuffer[frame * 2 + 1] = pcm;
        }

        std::size_t framesRemaining = kFramesPerBuffer;
        std::int16_t* data = backend_->mixBuffer.data();
        while (framesRemaining > 0 && running_) {
            snd_pcm_sframes_t written = snd_pcm_writei(backend_->handle, data, framesRemaining);
            if (written == -EPIPE) {
                snd_pcm_prepare(backend_->handle);
                continue;
            } else if (written < 0) {
                std::cerr << "AudioEngine: Error writing to ALSA device: "
                          << snd_strerror(static_cast<int>(written)) << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            framesRemaining -= static_cast<std::size_t>(written);
            data += written * kChannels;
        }
    }
#else
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#endif
}

