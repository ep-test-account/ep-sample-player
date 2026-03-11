#pragma once

#include <cstdint>

namespace EP {

enum class MidiMessageType : uint8_t
{
    NoteOnOff,
    ControlChange
};

/// Describes a single pad trigger: which MIDI message (type + channel + value)
/// maps to which pad index.
struct MidiMapping {
    int             padIndex; // 0..kNumPads-1
    MidiMessageType type;
    uint8_t         channel;  // 1–16
    uint8_t         value;    // note number (for type NoteOnOff) or CC number (for type ControlChange)
};

} // namespace EP
