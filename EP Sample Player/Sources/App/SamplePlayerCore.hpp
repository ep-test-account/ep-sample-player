#pragma once

#include "../Core/Types.hpp"
#include "../Core/Midi/MidiManager.hpp"
#include "../Core/Audio/AudioEngine.hpp"

#include "../Platform/CoreAudioOutput.h"
#include "../Platform/GDCTimer.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace EP {

/// Pure C++ class to act as the core application logic entry point, this aggregates both platform-agnostic and plaform speciflc classes (e.g. GDCTimer)
/// The plaform specifc stuff could be abstracted away with some Factories but was kept simple for the sake of the example!
class SamplePlayerCore {
public:
    using PadStateCallback = std::function<void (int padIndex, bool isPlaying)>;
    using MidiConfigCallback = MidiManager::ConfigChangedCallback;

    /// Construct with sample file paths (one per pad) and the MIDI mapping
    /// loaded from JSON. The mapping determines which MIDI messages trigger
    /// which pads.
    SamplePlayerCore(const std::vector<std::string>& samplePaths,
                     const std::string&              midiMappingPath,
                     MidiConfigCallback              midiConfigCB,
                     PadStateCallback                padStateCB);
    ~SamplePlayerCore();

    /// SamplePlayerCore is non-copyable
    SamplePlayerCore(const SamplePlayerCore&) = delete;
    SamplePlayerCore& operator=(const SamplePlayerCore&) = delete;

    /// Start audio output and begin polling engine state.
    void start();

    /// Stop audio output and polling.
    void stop();

    // ─── Pad control (UI → Engine via command ring buffer) ──────────
    int numberOfPads() const;
    void togglePad(int index);
    bool isPadPlaying(int index) const;
    std::string getPadMappingLabel(int index) const;
    void setPadVolume(int index, float volume);
    void setFadeEnabled(int index, bool enabled);
    void setMasterVolume(float volume);

    // ─── MIDI device management ─────────────────────────────────────

    int midiSourceCount() const;
    std::string midiSourceName(int index) const;
    bool connectMIDISource(int index);
    bool disconnectMIDISource(int index);
    bool isMIDISourceConnected(int index) const;

private:
    void loadSamples(const std::vector<std::string>& samplePaths);

    void pollEngineState();

    std::unique_ptr<AudioEngine> m_engine;
    std::unique_ptr<MidiManager> m_midiManager;

    std::unique_ptr<GDCTimer> m_gdcTimer;
    std::unique_ptr<CoreAudioOutput> m_coreAudioOutput;

    PadStateCallback  m_padStateCallback;
};

} // namespace EP
