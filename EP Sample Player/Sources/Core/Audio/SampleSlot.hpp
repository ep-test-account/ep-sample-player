#pragma once

#include "AudioBuffer.hpp"
#include "Renderable.hpp"

#include <atomic>
#include <vector>

namespace EP {

/// Represents a single playback voice for one pad.
///
/// Thread safety:
/// - play()/stop()/togglePlayback() are called from the audio render thread.
/// - render() is called exclusively from the audio render thread.
/// - setVolume()/setFadeEnabled() write atomic values that are read in render().
///
/// All mutable state accessed from the render thread is either:
/// - Exclusively owned by the render thread (m_playhead, m_fadeGain, m_fadeState)
/// - Atomic (m_volume, m_fadeEnabled)
class SampleSlot : public Renderable
{
public:
    SampleSlot() = default;

    /// Loads the sample slot with the provided audio buffer and info.
    /// Must be called before audio rendering starts.
    void load(std::vector<float>&& interleavedData, int numChannels, double sampleRate);

    /// Toggle playback: if stopped → start looping, if playing → stop.
    /// Can be called from the audio render thread.
    void togglePlayback();

    /// Explicit start/stop — Can be called from the audio render thread.
    void play();
    void stop();

    /// Returns true if the voice is currently playing or fading out.
    bool isPlaying() const;

    /// Set per-pad volume (0.0 – 1.0). Thread-safe (atomic).
    void setVolume(float volume);

    /// Returns the pad volume (0.0 – 1.0). Thread-safe (atomic).
    float getVolume() const;

    /// Enable/disable fade in/out. Thread-safe (atomic).
    void setFadeEnabled(bool enabled);

    /// Returns true if fading is enabled, false otherwise
    bool isFadeEnabled() const;

    /// Render this voice's output, adding into the destination buffers.
    /// Called exclusively from the audio render thread.
    ///
    /// @param buffer  Left/Right channel output buffer (additive)
    /// @param numFrames Number of frames to render
    void render(float* buffer[2], int numFrames) override;

private:
    void setPlaying(bool playing);

    // Fade state — only accessed from the audio render thread
    enum class FadeState
    {
        Idle,       // not fading
        FadingIn,   // ramping gain up from 0 to 1
        FadingOut   // ramping gain down from current to 0, then stop
    };

    FadeState m_fadeState  = FadeState::Idle;
    float     m_fadeGain   = 0.0f;

    int64_t   m_playhead  = 0;
    std::atomic<float> m_volume {1.0f};
    std::atomic<bool> m_playing {false};
    std::atomic<bool> m_fadeEnabled {true};

    std::unique_ptr<AudioBuffer> m_buffer;
};

} // namespace EP
