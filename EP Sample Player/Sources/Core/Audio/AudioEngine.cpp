#include "AudioEngine.hpp"

#include <algorithm>

namespace EP {

void AudioEngine::loadSample(int padIndex, std::vector<float>&& data, int channels, double sampleRate)
{
    if (isValidPadIndex(padIndex))
    {
        m_pads[padIndex].load(std::move(data), channels, sampleRate);
    }
}

void AudioEngine::render(float* buffer[2], int numFrames)
{
    // 1. Zero output buffers
    std::fill(buffer[0], buffer[0] + numFrames, 0.0f);
    std::fill(buffer[1], buffer[1] + numFrames, 0.0f);

    // 2. Drain incoming ring buffers and process commands
    processCommands(m_midiCommandRingBuffer);
    processCommands(m_uiCommandRingBuffer);

    // 3. Mix all active pads (additive)
    for (auto& pad : m_pads)
    {
        pad.render(buffer, numFrames);
    }

    // 4. Apply master volume and clamp
    const auto applyVolume = [masterVolume = getMasterVolume()] (float s) {
            return std::clamp(s * masterVolume, -1.0f, 1.0f);
        };
    std::transform(buffer[0], buffer[0] + numFrames, buffer[0], applyVolume);
    std::transform(buffer[1], buffer[1] + numFrames, buffer[1], applyVolume);
}

void AudioEngine::processCommand(const EngineCommand& cmd)
{
    switch (cmd.type)
    {
        case CommandType::TogglePad:
        {
            togglePad(cmd.padIndex);
            break;
        }

        case CommandType::SetPadVolume:
        {
            if (isValidPadIndex(cmd.padIndex))
            {
                m_pads[cmd.padIndex].setVolume(cmd.value);
            }
            break;
        }

        case CommandType::SetFadeEnabled:
        {
            if (isValidPadIndex(cmd.padIndex))
            {
                m_pads[cmd.padIndex].setFadeEnabled(cmd.value > 0.5f);
            }
            break;
        }

        case CommandType::SetMasterVolume:
        {
            setMasterVolume(cmd.value);
            break;
        }
    }
}

void AudioEngine::togglePad(int padIndex)
{
    if (isValidPadIndex(padIndex))
    {
        const auto wasPlaying = m_pads[padIndex].isPlaying();
        m_pads[padIndex].togglePlayback();
        const auto isPlaying = m_pads[padIndex].isPlaying();

        // Push state change event to UI ring buffer
        if (wasPlaying != isPlaying)
        {
            EngineEvent event{padIndex, isPlaying};
            m_uiEventRingBuffer.push(event);
        }
    }
}

bool AudioEngine::isPadPlaying(int padIndex) const
{
    if (isValidPadIndex(padIndex))
    {
        return m_pads[padIndex].isPlaying();
    }

    return false;
}

float AudioEngine::getMasterVolume() const
{
    return m_masterVolume.load(std::memory_order_relaxed);
}

void AudioEngine::setMasterVolume(float volume)
{
    m_masterVolume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}

} // namespace EP
