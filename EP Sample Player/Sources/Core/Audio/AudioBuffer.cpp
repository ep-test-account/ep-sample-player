#include "AudioBuffer.hpp"

namespace EP {
    
AudioBuffer::AudioBuffer(std::vector<float>&& interleavedData, int numChannels, double sampleRate)
    : m_data(std::move(interleavedData))
    , m_numChannels(numChannels)
    , m_sampleRate(sampleRate)
    , m_numFrames(numChannels > 0 ? static_cast<int64_t>(m_data.size()) / numChannels : 0)
{
}

} // namespace EP
