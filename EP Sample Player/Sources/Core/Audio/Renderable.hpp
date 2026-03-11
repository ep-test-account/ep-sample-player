#pragma once

namespace EP {

/// Interface for classes which can perform real-time audio rendering.
class Renderable {
public:
    /// Called from the audio real-time thread.
    ///
    /// @param buffer    Left/Right channel output buffer (non-interleaved)
    /// @param numFrames  Number of frames to render
    virtual void render(float* buffer[2], int numFrames) = 0;
};

} // namespace EP
