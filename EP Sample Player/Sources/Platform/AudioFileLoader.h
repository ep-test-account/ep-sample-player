#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Contains decoded audio data returned by AudioFileLoader_Load.
typedef struct {
    float*   data;          ///< Interleaved float PCM data. Caller must free via AudioFileLoader_Free.
    int64_t  numFrames;     ///< Total number of audio frames.
    int      numChannels;   ///< Number of channels (1 = mono, 2 = stereo).
    double   sampleRate;    ///< Sample rate of the decoded audio.
} LoadedAudioData;

/// Load an audio file and decode it to interleaved float PCM.
///
/// Supports any format readable by ExtAudioFile (WAV, AIFF, CAF, MP3, etc.).
/// The audio is automatically converted to the specified target sample rate
/// and to 32-bit float format.
///
/// @param filePath         Absolute path to the audio file
/// @param targetSampleRate Desired output sample rate (e.g., 44100.0)
/// @param outData          Output struct filled on success
/// @return 0 on success, non-zero on failure
int AudioFileLoader_Load(const char* filePath, double targetSampleRate, LoadedAudioData* outData);

/// Free the audio data allocated by AudioFileLoader_Load.
void AudioFileLoader_Free(LoadedAudioData* data);

#ifdef __cplusplus
}
#endif
