#pragma once

#include <vector>
#include <cstdint>

namespace EP {

/// Holds decoded audio sample data as interleaved 32-bit float PCM.
///
/// Once loaded, the buffer data is immutable and safe to read from
/// the real-time audio thread without synchronization.
class AudioBuffer
{
public:
    /// Constructs an audio buffer with the provided audio data.
    ///
    /// @param interleavedData  Interleaved float PCM samples
    /// @param numChannels      Number of channels (1 = mono, 2 = stereo)
    /// @param sampleRate       Sample rate of the audio data
    AudioBuffer(std::vector<float>&& interleavedData, int numChannels, double sampleRate);

    bool isLoaded() const { return m_numFrames > 0; }

    const float* data() const { return m_data.data(); }
    int64_t numFrames() const { return m_numFrames; }
    int numChannels() const { return m_numChannels; }
    double sampleRate() const { return m_sampleRate; }

private:
    const std::vector<float> m_data;
    const int m_numChannels = 0;
    const int64_t m_numFrames = 0;
    const double m_sampleRate = 0.0;
};

} // namespace EP
