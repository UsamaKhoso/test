#pragma once

#include <atomic>
#include <thread>

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool initialize();
    void start();
    void stop();
    void shutdown();

    bool isRunning() const { return running_; }

private:
    void audioThread();

    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    std::thread worker_;

#if defined(__linux__) && !defined(SIMPLE3D_NO_ALSA)
    struct LinuxBackend;
    LinuxBackend* backend_{nullptr};
#endif
};

