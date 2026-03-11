#include "CoreAudioOutput.h"

#include <CoreAudio/CoreAudio.h>

#include <cstdio>
#include <cstring>
#include <vector>

struct CoreAudioOutputData {
    AudioDeviceID       deviceID;
    AudioDeviceIOProcID ioProcID;
    EP::Renderable*     renderable;
    bool                running;
    bool                interleaved;   // true if device uses interleaved stereo
    int                 numChannels;   // actual channel count from device
};

namespace detail {

// CoreAudio HAL IOProc
static OSStatus ioProc(
    AudioDeviceID           device,
    const AudioTimeStamp*   now,
    const AudioBufferList*  inputData,
    const AudioTimeStamp*   inputTime,
    AudioBufferList*        outputData,
    const AudioTimeStamp*   outputTime,
    void*                   clientData)
{
    auto* data = static_cast<CoreAudioOutputData*>(clientData);

    if (!data || !data->renderable || !outputData || outputData->mNumberBuffers == 0)
    {
        if (outputData)
        {
            for (UInt32 i = 0; i < outputData->mNumberBuffers; ++i)
            {
                std::memset(outputData->mBuffers[i].mData, 0,
                            outputData->mBuffers[i].mDataByteSize);
            }
        }

        return noErr;
    }

    if (!data->interleaved && outputData->mNumberBuffers >= 2)
    {
        // Non-interleaved: separate L/R buffers — render directly
        const int numFrames  = static_cast<int>(outputData->mBuffers[0].mDataByteSize / sizeof(float));

        float* buffers[2] = {
          static_cast<float*>(outputData->mBuffers[0].mData),
          static_cast<float*>(outputData->mBuffers[1].mData)
        };

        data->renderable->render(buffers, numFrames);
    }
    else
    {
        // Interleaved: single buffer with L/R samples alternating
        const int totalSamples = static_cast<int>(outputData->mBuffers[0].mDataByteSize / sizeof(float));
        const int channels = std::max(data->numChannels, 2);
        const int numFrames = totalSamples / channels;

        // Render into temporary non-interleaved buffers, then interleave
        // Use stack allocation for typical buffer sizes (up to 4096 frames), heap for bigger sizes
        float tempL[4096];
        float tempR[4096];
        float* pL = tempL;
        float* pR = tempR;

        std::vector<float> heapL, heapR;
        if (numFrames > 4096)
        {
            heapL.resize(numFrames);
            heapR.resize(numFrames);
            pL = heapL.data();
            pR = heapR.data();
        }

        float* buffers[2] = { pL, pR };
        data->renderable->render(buffers, numFrames);

        // Interleave into output buffer
        float* interleavedBuf = static_cast<float*>(outputData->mBuffers[0].mData);

        for (int i = 0; i < numFrames; ++i)
        {
            interleavedBuf[i * channels]     = pL[i];
            interleavedBuf[i * channels + 1] = pR[i];

            // Zero any extra channels beyond stereo
            for (int ch = 2; ch < channels; ++ch)
            {
                interleavedBuf[i * channels + ch] = 0.0f;
            }
        }
    }

    return noErr;
}

} // namespace detail

CoreAudioOutput::CoreAudioOutput(double sampleRate, EP::Renderable* renderable)
    : m_data(std::make_unique<CoreAudioOutputData>())
{
    // 1. Get the default output device
    AudioDeviceID deviceID = kAudioObjectUnknown;
    UInt32 propSize = sizeof(deviceID);

    AudioObjectPropertyAddress defaultOutputAddr = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMain
    };

    OSStatus status = AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &defaultOutputAddr,
        0, nullptr,
        &propSize,
        &deviceID
    );

    if (status != noErr || deviceID == kAudioObjectUnknown)
    {
        throw std::runtime_error("CoreAudioOutput: Failed to get default output device");
    }

    // 2. Try to set stream format: non-interleaved 32-bit float stereo
    AudioStreamBasicDescription desiredFormat = {};
    desiredFormat.mSampleRate       = sampleRate;
    desiredFormat.mFormatID         = kAudioFormatLinearPCM;
    desiredFormat.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved;
    desiredFormat.mBytesPerPacket   = sizeof(float);
    desiredFormat.mFramesPerPacket  = 1;
    desiredFormat.mBytesPerFrame    = sizeof(float);
    desiredFormat.mChannelsPerFrame = 2;
    desiredFormat.mBitsPerChannel   = 32;

    AudioObjectPropertyAddress formatAddr = {
        kAudioDevicePropertyStreamFormat,
        kAudioObjectPropertyScopeOutput,
        kAudioObjectPropertyElementMain
    };

    status = AudioObjectSetPropertyData(
        deviceID,
        &formatAddr,
        0, nullptr,
        sizeof(desiredFormat),
        &desiredFormat
    );

    if (status != noErr)
    {
        throw new std::runtime_error("CoreAudioOutput: Could not set desired format");
    }

    // 3. Read back actual format to know what the device is using
    AudioStreamBasicDescription actualFormat = {};
    propSize = sizeof(actualFormat);
    status = AudioObjectGetPropertyData(
        deviceID,
        &formatAddr,
        0, nullptr,
        &propSize,
        &actualFormat
    );

    if (status != noErr)
    {
        throw new std::runtime_error("CoreAudioOutput: Failed to query device format");
    }

    const bool isNonInterleaved = (actualFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved) != 0;

    std::fprintf(stderr, "CoreAudioOutput: Device format — %.0f Hz, %d ch, %s, %d bits\n",
                 actualFormat.mSampleRate,
                 (int)actualFormat.mChannelsPerFrame,
                 isNonInterleaved ? "non-interleaved" : "interleaved",
                 (int)actualFormat.mBitsPerChannel);

    // 4. Create context and install IOProc
    m_data->deviceID    = deviceID;
    m_data->ioProcID    = nullptr;
    m_data->renderable = renderable;
    m_data->running     = false;
    m_data->interleaved = !isNonInterleaved;
    m_data->numChannels = static_cast<int>(actualFormat.mChannelsPerFrame);

    status = AudioDeviceCreateIOProcID(deviceID, detail::ioProc, m_data.get(), &m_data->ioProcID);

    if (status != noErr)
    {
        throw new std::runtime_error("CoreAudioOutput: Failed to create IOProc");
    }
}

CoreAudioOutput::~CoreAudioOutput()
{
    if (m_data)
    {
        if (m_data->running)
        {
            AudioDeviceStop(m_data->deviceID, m_data->ioProcID);
        }

        if (m_data->ioProcID)
        {
            AudioDeviceDestroyIOProcID(m_data->deviceID, m_data->ioProcID);
        }
    }
}

void CoreAudioOutput::start()
{
    if (m_data && !m_data->running)
    {
      OSStatus status = AudioDeviceStart(m_data->deviceID, m_data->ioProcID);

      if (status != noErr)
      {
          std::fprintf(stderr, "CoreAudioOutput: Failed to start device: %d\n", (int)status);
          return;
      }

      m_data->running = true;
    }
}

void CoreAudioOutput::stop()
{
    if (m_data && m_data->running)
    {
      AudioDeviceStop(m_data->deviceID, m_data->ioProcID);
      m_data->running = false;
    }
}
