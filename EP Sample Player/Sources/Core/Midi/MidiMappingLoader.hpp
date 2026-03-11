#pragma once

#include "MidiMapping.hpp"

#include <string>
#include <vector>

namespace EP {

/// Loads a MIDI mapping from a JSON file.
///
/// Throws std::runtime_error if the file cannot be opened or the JSON is
/// malformed / missing required fields.
///
/// Expected JSON format:
/// @code
/// {
///   "mappings": [
///     { "pad": 0, "type": "note",          "channel": 1, "value": 36 },
///     { "pad": 1, "type": "cc",            "channel": 1, "value": 20 }
///   ]
/// }
/// @endcode
///
/// Rules:
///  - "type" must be "note" or "cc"
///  - "channel" must be 1–16
///  - "value" must be 0–127
///  - "pad" must be 0–(kNumPads-1)
///  - Duplicate pad entries are allowed; last one wins at parse time
///    (MIDIManager iterates in order and uses first match at runtime)
class MidiMappingLoader {
public:
    static std::vector<MidiMapping> load(const std::string& filePath);
};

} // namespace EP
