#pragma once

#include "../Core/Audio/Renderable.hpp"

#include <memory>

class CoreAudioOutput
{
public:
    /// Create a CoreAudio output host using the HAL API.
    ///
    /// @param sampleRate  Desired sample rate (e.g., 44100.0)
    /// @param renderable   Pointer to a Renderable to render audio
    CoreAudioOutput(double sampleRate, EP::Renderable* renderable);

    ~CoreAudioOutput();

    void start();
    void stop();

private:
    /// Opaque handle to the CoreAudioOutput private data (to avoid including CoreAudio in this header)
    std::unique_ptr<struct CoreAudioOutputData> m_data;
};
