#pragma once

#include "../Core/Midi/MidiManager.hpp"

#include <string>
#include <vector>
#include <memory>

namespace EP {

/// Implements a MidiManager using CoreMidi API
class CoreMidiManager : public MidiManager
{
public:
    CoreMidiManager(EventCallback eventCB, ConfigChangedCallback configCB, const std::string& mappingsPath);
    ~CoreMidiManager();

    /// Implement MidiManager virtual methods
    int sourceCount() const override;
    std::string sourceName(int index) const override;
    bool connectSource(int index) override;
    bool disconnectSource(int index) override;
    bool isSourceConnected(int index) const override;

private:
    /// Opaque handle to the CoreMidiManager private data (to avoid including CoreMIDI in this header)
    std::unique_ptr<struct CoreMidiManagerData> m_data;
};

} // namespace EP
