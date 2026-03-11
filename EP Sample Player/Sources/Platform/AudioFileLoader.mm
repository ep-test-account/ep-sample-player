#import "AudioFileLoader.h"
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#include <vector>

int AudioFileLoader_Load(const char* filePath, double targetSampleRate, LoadedAudioData* outData) {
    if (!filePath || !outData) return -1;

    // Zero output
    outData->data = nullptr;
    outData->numFrames = 0;
    outData->numChannels = 0;
    outData->sampleRate = 0.0;

    // Open the audio file
    NSString* path = [NSString stringWithUTF8String:filePath];
    NSURL* fileURL = [NSURL fileURLWithPath:path];

    ExtAudioFileRef audioFile = nullptr;
    OSStatus status = ExtAudioFileOpenURL((__bridge CFURLRef)fileURL, &audioFile);
    if (status != noErr) {
        NSLog(@"AudioFileLoader: Failed to open file '%s': %d", filePath, (int)status);
        return -1;
    }

    // Get the file's native format to determine channel count
    AudioStreamBasicDescription fileFormat = {};
    UInt32 propSize = sizeof(fileFormat);
    status = ExtAudioFileGetProperty(
        audioFile,
        kExtAudioFileProperty_FileDataFormat,
        &propSize,
        &fileFormat
    );
    if (status != noErr) {
        NSLog(@"AudioFileLoader: Failed to get file format: %d", (int)status);
        ExtAudioFileDispose(audioFile);
        return -1;
    }

    const int numChannels = fileFormat.mChannelsPerFrame;

    // Set the client (output) format: interleaved 32-bit float at target sample rate
    AudioStreamBasicDescription clientFormat = {};
    clientFormat.mSampleRate       = targetSampleRate;
    clientFormat.mFormatID         = kAudioFormatLinearPCM;
    clientFormat.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    clientFormat.mBytesPerPacket   = sizeof(float) * numChannels;
    clientFormat.mFramesPerPacket  = 1;
    clientFormat.mBytesPerFrame    = sizeof(float) * numChannels;
    clientFormat.mChannelsPerFrame = numChannels;
    clientFormat.mBitsPerChannel   = 32;

    status = ExtAudioFileSetProperty(
        audioFile,
        kExtAudioFileProperty_ClientDataFormat,
        sizeof(clientFormat),
        &clientFormat
    );
    if (status != noErr) {
        NSLog(@"AudioFileLoader: Failed to set client format: %d", (int)status);
        ExtAudioFileDispose(audioFile);
        return -1;
    }

    // Get total frame count (in client format, after sample rate conversion)
    SInt64 totalFrames = 0;
    propSize = sizeof(totalFrames);
    status = ExtAudioFileGetProperty(
        audioFile,
        kExtAudioFileProperty_FileLengthFrames,
        &propSize,
        &totalFrames
    );
    if (status != noErr || totalFrames <= 0) {
        NSLog(@"AudioFileLoader: Failed to get frame count: %d", (int)status);
        ExtAudioFileDispose(audioFile);
        return -1;
    }

    // Adjust for sample rate conversion: the frame count from the file property
    // is in source sample rate frames. After conversion, it scales proportionally.
    double ratio = targetSampleRate / fileFormat.mSampleRate;
    SInt64 estimatedOutputFrames = static_cast<SInt64>(totalFrames * ratio) + 1024; // extra margin

    // Read all audio data
    std::vector<float> buffer;
    buffer.reserve(estimatedOutputFrames * numChannels);

    const UInt32 readChunkFrames = 4096;
    std::vector<float> readBuffer(readChunkFrames * numChannels);

    while (true) {
        AudioBufferList bufferList = {};
        bufferList.mNumberBuffers = 1;
        bufferList.mBuffers[0].mNumberChannels = numChannels;
        bufferList.mBuffers[0].mDataByteSize = static_cast<UInt32>(readBuffer.size() * sizeof(float));
        bufferList.mBuffers[0].mData = readBuffer.data();

        UInt32 framesToRead = readChunkFrames;
        status = ExtAudioFileRead(audioFile, &framesToRead, &bufferList);
        if (status != noErr) {
            NSLog(@"AudioFileLoader: Error reading audio data: %d", (int)status);
            ExtAudioFileDispose(audioFile);
            return -1;
        }

        if (framesToRead == 0) {
            break; // end of file
        }

        buffer.insert(
            buffer.end(),
            readBuffer.begin(),
            readBuffer.begin() + framesToRead * numChannels
        );
    }

    ExtAudioFileDispose(audioFile);

    if (buffer.empty()) {
        NSLog(@"AudioFileLoader: File contained no audio data");
        return -1;
    }

    // Copy to output
    const int64_t actualFrames = static_cast<int64_t>(buffer.size()) / numChannels;
    outData->data = static_cast<float*>(malloc(buffer.size() * sizeof(float)));
    if (!outData->data) {
        NSLog(@"AudioFileLoader: Failed to allocate output buffer");
        return -1;
    }

    memcpy(outData->data, buffer.data(), buffer.size() * sizeof(float));
    outData->numFrames = actualFrames;
    outData->numChannels = numChannels;
    outData->sampleRate = targetSampleRate;

    return 0;
}

void AudioFileLoader_Free(LoadedAudioData* data) {
    if (data && data->data) {
        free(data->data);
        data->data = nullptr;
        data->numFrames = 0;
    }
}
