#include "MidiManager.hpp"

#include "MidiMappingLoader.hpp"

#include <algorithm>

namespace EP {

MidiManager::MidiManager(EventCallback eventCB, ConfigChangedCallback configChangedCB, const std::string& mappingsPath)
    : m_eventCB(std::move(eventCB))
    , m_configChangedCB(std::move(configChangedCB))
{
    if (mappingsPath.empty())
    {
        throw new std::runtime_error("midi_mapping.json was not found!");
    }

    m_mappings = MidiMappingLoader::load(mappingsPath);
}

void MidiManager::handleMIDIMessage(uint8_t status, uint8_t data1, uint8_t data2)
{
    if (!m_eventCB)
        return;

    const uint8_t type = status & 0xF0;
    const uint8_t channel = (status & 0x0F) + 1; // convert to 1-based

    const auto pred = [type, channel, data1, data2] (const MidiMapping& m) {
            if ((m.channel == channel) &&
               ((type == 0x90 && m.type == MidiMessageType::NoteOnOff) ||
                (type == 0xB0 && m.type == MidiMessageType::ControlChange)))
            {
                return data1 == m.value && data2 > 0;
            }

            // Fallback
            return false;
        };

    const auto it = std::find_if(m_mappings.begin(), m_mappings.end(), pred);

    if (it != m_mappings.end())
    {
        const auto padIndex = static_cast<int8_t>(it->padIndex);
        EngineCommand cmd { CommandType::TogglePad, padIndex, 0.0f };

        m_eventCB(cmd);
    }
}

void MidiManager::handleMIDIConfigChanged()
{
    if (m_configChangedCB)
    {
        m_configChangedCB();
    }
}

// ─── Label helpers ───────────────────────────────────────────────────────────

std::string MidiManager::getPadMappingLabel(int index) const
{
    const auto pred = [index] (const MidiMapping& m) { return m.padIndex == index; };
    const auto it = std::find_if(m_mappings.begin(), m_mappings.end(), pred);

    if (it != m_mappings.end())
    {
        return labelForMapping(*it);
    }
    
    return {};
}

std::string MidiManager::noteToName(uint8_t note)
{
    static constexpr const char* names[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };
    const int octave = note / 12 - 2; // C1 = 36, C3 = 60 (middle C)
    return std::string(names[note % 12]) + std::to_string(octave);
}

std::string MidiManager::labelForMapping(const MidiMapping& mapping)
{
    if (mapping.type == MidiMessageType::NoteOnOff)
        return noteToName(mapping.value);

    return "CC " + std::to_string(mapping.value);
}

} // namespace EP
