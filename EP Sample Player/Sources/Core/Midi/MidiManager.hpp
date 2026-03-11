#pragma once

#include "Types.hpp"
#include "MidiMapping.hpp"

#include <string>
#include <vector>
#include <functional>

namespace EP {

/// Parses raw MIDI messages and maps them to application commands forwarded via a callback.
/// This class contains no platform-specific code. Derived class will add platform-specific APIs (e.g. CoreMIDI)
class MidiManager
{
public:
    /// Callback type for forwarding incoming MIDI events.
    /// Must be lock-free and allocation-free (called from the real-time thread).
    using EventCallback = std::function<void (const EngineCommand& command)>;

    /// Callback type for notifying of MIDI configuration changes
    using ConfigChangedCallback = std::function<void ()>;

    /// Construct with pad mappings and the callbacks invoked when a matching MIDI event is parsed and MIDI configuration changed.
    /// The mappings are copied and owned by this object.
    MidiManager(EventCallback eventCB, ConfigChangedCallback configChangedCB, const std::string& mappingsPath);

    /// Handles an incoming raw MIDI message.
    /// @param status  MIDI status byte
    /// @param data1   First data byte
    /// @param data2   Second data byte
    void handleMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2);

    /// Triggers with MIDI configuration changed callback.
    void handleMIDIConfigChanged();

    /// Get the number of available MIDI sources.
    virtual int sourceCount() const = 0;

    /// Get the display name of a MIDI source by index.
    virtual std::string sourceName(int index) const = 0;

    /// Connect a MIDI source by index to the input port. Returns true on success, false on failure.
    virtual bool connectSource(int index) = 0;
    
    /// Disconnect a MIDI source by index from the input port. Returns true on success, false on failure.
    virtual bool disconnectSource(int index) = 0;
    
    /// Check if a MIDI source is currently connected. Returns true if connected, false if not.
    virtual bool isSourceConnected(int index) const = 0;

    // ─── Label helpers ────────
    
    std::string getPadMappingLabel(int index) const;

    /// Converts a MIDI note number to a human-readable name, e.g. 36 → "C1".
    /// Convention: note 0 = C-2, note 36 = C1, note 60 = C3 (middle C).
    static std::string noteToName(uint8_t note);

    /// Returns a short display label for a single mapping entry,
    /// e.g. "C1" for a NoteOnOff or "CC 20" for a ControlChange.
    static std::string labelForMapping(const MidiMapping& mapping);

private:
    const EventCallback m_eventCB;
    const ConfigChangedCallback m_configChangedCB;
    std::vector<MidiMapping> m_mappings;

};

} // namespace EP
