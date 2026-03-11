#include "SamplePlayerCore.hpp"

#include "../Platform/AudioFileLoader.h"
#include "../Platform/CoreMidiManager.h"

#include <algorithm>
#include <cstdio>

namespace EP {

SamplePlayerCore::SamplePlayerCore(const std::vector<std::string>& samplePaths,
                                   const std::string&              midiMappingPath,
                                   MidiConfigCallback              midiConfigCB,
                                   PadStateCallback                padStateCB)
    : m_padStateCallback(std::move(padStateCB))
{
    m_engine = std::make_unique<AudioEngine>();

    // Wire MIDI manager to push events into the engine
    m_midiManager = std::make_unique<CoreMidiManager>(
        [engine = m_engine.get()] (const EngineCommand& command) {
            engine->pushMIDICommand(command);
        },
        std::move(midiConfigCB),
        midiMappingPath);

    m_coreAudioOutput = std::make_unique<CoreAudioOutput>(kSampleRate, m_engine.get());
    m_gdcTimer = std::make_unique<GDCTimer>([this] { pollEngineState(); });

  loadSamples(samplePaths);
}

SamplePlayerCore::~SamplePlayerCore()
{
    stop();
}

void SamplePlayerCore::start()
{
    if (m_coreAudioOutput)
    {
        m_coreAudioOutput->start();
    }

    if (m_gdcTimer)
    {
        m_gdcTimer->start();
    }
}

void SamplePlayerCore::stop()
{
    if (m_gdcTimer)
    {
        m_gdcTimer->stop();
    }

    if (m_coreAudioOutput)
    {
        m_coreAudioOutput->stop();
    }
}

void SamplePlayerCore::pollEngineState()
{
    EngineEvent event;
    while (m_engine->popUIEvent(event))
    {
        if (m_padStateCallback)
        {
            m_padStateCallback(event.padIndex, event.isPlaying);
        }
    }
}

int SamplePlayerCore::numberOfPads() const
{
    return AudioEngine::numberOfPads();
}

bool SamplePlayerCore::isPadPlaying(int index) const
{
    if (m_engine)
    {
        return m_engine->isPadPlaying(index);
    }

    return false;
}

std::string SamplePlayerCore::getPadMappingLabel(int index) const
{
    if (m_engine)
    {
        return m_midiManager->getPadMappingLabel(index);
    }

    return {};
}

// UI Methods

void SamplePlayerCore::togglePad(int index)
{
    if (m_engine)
    {
        EngineCommand cmd { CommandType::TogglePad, index, 0.0f };
        m_engine->pushUICommand(cmd);
    }
}

void SamplePlayerCore::setPadVolume(int index, float volume)
{
    if (m_engine)
    {
        EngineCommand cmd { CommandType::SetPadVolume, index, volume };
        m_engine->pushUICommand(cmd);
    }
}

void SamplePlayerCore::setFadeEnabled(int index, bool enabled)
{
    if (m_engine)
    {
        EngineCommand cmd { CommandType::SetFadeEnabled, index, enabled ? 1.0f : 0.0f };
        m_engine->pushUICommand(cmd);
    }
}

void SamplePlayerCore::setMasterVolume(float volume)
{
    if (m_engine)
    {
        EngineCommand cmd { CommandType::SetMasterVolume, 0, volume };
        m_engine->pushUICommand(cmd);
    }
}

// MIDI Sources management

int SamplePlayerCore::midiSourceCount() const
{
    if (m_midiManager)
    {
        return m_midiManager->sourceCount();
    }

    return 0;
}

std::string SamplePlayerCore::midiSourceName(int index) const
{
    if (m_midiManager)
    {
        return m_midiManager->sourceName(index);
    }

    return {};
}

bool SamplePlayerCore::connectMIDISource(int index)
{
    if (m_midiManager)
    {
        return m_midiManager->connectSource(index);
    }

    return false;
}

bool SamplePlayerCore::disconnectMIDISource(int index)
{
    if (m_midiManager)
    {
        return m_midiManager->disconnectSource(index);
    }

    return false;
}

bool SamplePlayerCore::isMIDISourceConnected(int index) const
{
    if (m_midiManager)
    {
        return m_midiManager->isSourceConnected(index);
    }

    return false;
}

// Sample loading

void SamplePlayerCore::loadSamples(const std::vector<std::string>& samplePaths)
{
    int padIndex = 0;
    for (const auto& path : samplePaths)
    {
        if (!path.empty())
        {
            LoadedAudioData audioData;
            if (AudioFileLoader_Load(path.c_str(), kSampleRate, &audioData) == 0)
            {
                std::vector<float> buffer(audioData.data, audioData.data + audioData.numFrames * audioData.numChannels);
                m_engine->loadSample(padIndex, std::move(buffer), audioData.numChannels, audioData.sampleRate);
                AudioFileLoader_Free(&audioData);
                std::printf("SamplePlayerCore: Loaded sample %d: %s\n", padIndex, path.c_str());
            }
            else
            {
                std::printf("SamplePlayerCore: Failed to load sample %d: %s\n", padIndex, path.c_str());
            }
        }
        else
        {
            std::printf("SamplePlayerCore: Empty path for pad %d, skipping\n", padIndex);
        }

        padIndex++;
    }
}

} // namespace EP
