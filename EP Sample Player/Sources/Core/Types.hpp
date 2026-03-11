#pragma once

#include <cstdint>

namespace EP {

// ─── Constants ───────────────────────────────────────────────────────────────

constexpr int    kNumPads       = 8;
constexpr double kSampleRate    = 44100.0;

// Fade duration in samples (~5ms at 44.1kHz)
constexpr int    kFadeDurationSamples = 220;

// ─── Message types for ring buffers ──────────────────────────────────────────

// Command types sent from the UI to the engine.
enum class CommandType : uint8_t {
    TogglePad,
    SetPadVolume,
    SetMasterVolume,
    SetFadeEnabled
};

// Command pushed by the UI into the command ring buffer.
struct EngineCommand
{
    CommandType type;
    int         padIndex;   // relevant for TogglePad, SetPadVolume, SetFadeEnabled
    float       value;      // volume (0..1) or fade enabled (0=false, 1=true)
};

// Event pushed by the engine into the UI event ring buffer.
struct EngineEvent
{
    EngineEvent() = default;
    EngineEvent(int pad, bool playing)
        : padIndex(pad), isPlaying(playing) {}

    int  padIndex;
    bool isPlaying;
};

} // namespace EP
