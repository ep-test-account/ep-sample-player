#include "SampleSlot.hpp"
#include "Types.hpp"

#include <algorithm>

namespace EP {

void SampleSlot::load(std::vector<float>&& interleavedData, int numChannels, double sampleRate)
{
    m_buffer = std::make_unique<AudioBuffer>(std::move(interleavedData), numChannels, sampleRate);
}

void SampleSlot::togglePlayback()
{
    if (isPlaying())
    {
        stop();
    }
    else
    {
        play();
    }
}

void SampleSlot::play()
{
    m_playhead = 0;
    setPlaying(true);

    if (isFadeEnabled())
    {
        m_fadeGain = 0.0f;
        m_fadeState = FadeState::FadingIn;
    }
    else
    {
        m_fadeGain = 1.0f;
        m_fadeState = FadeState::Idle;
    }
}

void SampleSlot::stop()
{
    setPlaying(false);

    if (isFadeEnabled())
    {
        // Begin fade-out from current fade-gain level
        m_fadeState = FadeState::FadingOut;
    }
    else
    {
        m_fadeGain = 0.0f;
        m_fadeState = FadeState::Idle;
    }
}

void SampleSlot::setPlaying(bool playing)
{
    m_playing.store(playing, std::memory_order_relaxed);
}

bool SampleSlot::isPlaying() const
{
    return m_playing.load(std::memory_order_relaxed);
}

void SampleSlot::setVolume(float volume)
{
    m_volume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}

float SampleSlot::getVolume() const
{
    return m_volume.load(std::memory_order_relaxed);
}

void SampleSlot::setFadeEnabled(bool enabled)
{
    m_fadeEnabled.store(enabled, std::memory_order_relaxed);
}

bool SampleSlot::isFadeEnabled() const
{
  return m_fadeEnabled.load(std::memory_order_relaxed);
}

void SampleSlot::render(float* buffer[2], int numFrames)
{
    constexpr float  kFadeStepPerSample = 1.0f / static_cast<float>(kFadeDurationSamples);

    if (!m_buffer || !m_buffer->isLoaded())
    {
        // There's no buffer to render!
        return;
    }

    if (!isPlaying() && m_fadeState == FadeState::Idle)
    {
        // The sample is not playing!
        return;
    }

    const float* src = m_buffer->data();
    const int64_t totalFrames = m_buffer->numFrames();
    const float volume = getVolume();

    for (int i = 0; i < numFrames; ++i)
    {
        switch (m_fadeState)
        {
            case FadeState::FadingIn:
            {
                m_fadeGain += kFadeStepPerSample;
                if (m_fadeGain >= 1.0f)
                {
                    m_fadeGain = 1.0f;
                    m_fadeState = FadeState::Idle;
                }

                break;
            }

            case FadeState::FadingOut:
            {
                m_fadeGain -= kFadeStepPerSample;
                if (m_fadeGain <= 0.0f)
                {
                    m_fadeGain = 0.0f;
                    m_fadeState = FadeState::Idle;

                    return; // done fading out, stop rendering
                }

                break;
            }

            case FadeState::Idle:
            {
                break;
            }
        }

        // Read sample data with loop wraparound
        const float gain = volume * m_fadeGain;

        if (m_buffer->numChannels() == 2)
        {
            buffer[0][i] += src[m_playhead * 2]     * gain;
            buffer[1][i] += src[m_playhead * 2 + 1] * gain;
        }
        else
        {
            // Mono: duplicate to both channels
            const float sample = src[m_playhead] * gain;
            buffer[0][i] += sample;
            buffer[1][i] += sample;
        }

        m_playhead = (m_playhead + 1) % totalFrames; // wraps via modulo on next iteration
    }
}

} // namespace EP
