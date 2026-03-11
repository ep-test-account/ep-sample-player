#pragma once

#include "SampleSlot.hpp"
#include "RingBuffer.hpp"
#include "Renderable.hpp"
#include "Types.hpp"

#include <array>
#include <atomic>

namespace EP {

/// Central audio engine that owns all sample voices and orchestrates real-time audio rendering.
///
/// This class runs entirely on the audio render thread. It drains
/// incoming ring buffers (MIDI events, UI commands), updates voice state,
/// mixes all active voices, and pushes state-change events back to the UI.
class AudioEngine : public Renderable {
public:
    static constexpr int numberOfPads() { return kNumPads; }
    static constexpr bool isValidPadIndex(int index) { return (index >= 0 && index < kNumPads); }

    AudioEngine() = default;
    ~AudioEngine() = default;

    /// Load a sample into a pad slot. Must be called before rendering begins.
    void loadSample(int padIndex, std::vector<float>&& data, int channels, double sampleRate);

    /// Push a command from the UI thread (lock-free, non-blocking).
    inline void pushUICommand(const EngineCommand& cmd) { m_uiCommandRingBuffer.push(cmd); }

    /// Push a MIDI event from the CoreMIDI callback (lock-free, real-time safe).
    inline void pushMIDICommand(const EngineCommand& cmd) { m_midiCommandRingBuffer.push(cmd); }

    /// Pop the next UI event (lock-free). Returns true if an event was available.
    inline bool popUIEvent(EngineEvent& event) { return m_uiEventRingBuffer.pop(event); }

    /// Main render function. Called from the CoreAudio real-time thread.
    /// Drains ring buffers, mixes all voices, applies master volume.
    ///
    /// @param buffer    Left/Right channel output buffer (non-interleaved)
    /// @param numFrames  Number of frames to render
    void render(float* buffer[2], int numFrames) override;

    void togglePad(int padIndex);
    bool isPadPlaying(int index) const;

    float getMasterVolume() const;
    void setMasterVolume(float volume);

private:
    template <typename EventBuffer>
    void processCommands(EventBuffer& buffer)
    {
        EngineCommand cmd;
        while (buffer.pop(cmd))
        {
            processCommand(cmd);
        }
    }
    
    void processCommand(const EngineCommand& cmd);

    std::array<SampleSlot, kNumPads> m_pads;
    std::atomic<float> m_masterVolume {1.0f};

    // Ring buffers for lock-free communication
    RingBuffer<EngineCommand, 256> m_midiCommandRingBuffer;
    RingBuffer<EngineCommand, 256> m_uiCommandRingBuffer;
    RingBuffer<EngineEvent,   256> m_uiEventRingBuffer;
};

} // namespace EP
