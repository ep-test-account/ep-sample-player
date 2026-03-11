#include "MidiMappingLoader.hpp"
#include "Types.hpp"

#include "../../External/json.hpp"

#include <fstream>
#include <string>

namespace EP {

std::vector<MidiMapping> MidiMappingLoader::load(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open MIDI mapping file: " + filePath);
    }

    nlohmann::json root;
    try
    {
        file >> root;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        throw std::runtime_error(std::string("MIDI mapping JSON parse error: ") + e.what());
    }

    // There must be a root "mapping" element
    if (!root.contains("mappings") || !root["mappings"].is_array())
    {
        throw std::runtime_error("MIDI mapping JSON must contain a top-level \"mappings\" array");
    }

    std::vector<MidiMapping> result;

    for (const auto& entry : root["mappings"])
    {
        // Check for required fields...
        if (!entry.contains("pad") || !entry.contains("type") || !entry.contains("channel"))
        {
            throw std::runtime_error("Each pad entry must have \"pad\", \"type\", and \"channel\"");
        }

        MidiMapping mapping;
        mapping.padIndex = entry["pad"].get<int>();

        if (mapping.padIndex < 0 || mapping.padIndex >= kNumPads)
        {
            throw std::runtime_error("\"pad\" value out of range (0–" +
                                     std::to_string(kNumPads - 1) + "): " +
                                     std::to_string(mapping.padIndex));
        }

        mapping.channel = static_cast<uint8_t>(entry["channel"].get<int>());

        if (mapping.channel < 1 || mapping.channel > 16)
        {
            throw std::runtime_error("\"channel\" value out of range (1–16): " +
                                     std::to_string(mapping.channel));
        }

        const auto typeStr = entry["type"].get<std::string>();

        if (typeStr == "note")
        {
            mapping.type  = MidiMessageType::NoteOnOff;

            if (!entry.contains("value"))
            {
                throw std::runtime_error("note entry for pad " +
                                         std::to_string(mapping.padIndex) + " is missing \"value\"");
            }

            const auto note = entry["value"].get<int>();
            if (note < 0 || note > 127)
            {
                throw std::runtime_error("\"value\" must be 0–127, got: " + std::to_string(note));
            }

            mapping.value = static_cast<uint8_t>(note);
        }
        else if (typeStr == "cc")
        {
            mapping.type  = MidiMessageType::ControlChange;

            if (!entry.contains("value"))
            {
                throw std::runtime_error("cc entry for pad " +
                                         std::to_string(mapping.padIndex) + " is missing \"value\"");
            }

            const auto cc = entry["value"].get<int>();
            if (cc < 0 || cc > 127)
            {
                throw std::runtime_error("\"value\" must be 0–127, got: " + std::to_string(cc));
            }

            mapping.value = static_cast<uint8_t>(cc);
        }
        else
        {
            throw std::runtime_error("Unknown MIDI message type \"" + typeStr +
                                     "\" (expected \"note\" or \"cc\")");
        }

        result.push_back(mapping);
    }

    return result;
}

} // namespace EP
