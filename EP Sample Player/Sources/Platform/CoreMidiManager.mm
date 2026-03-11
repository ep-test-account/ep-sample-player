#import "CoreMidiManager.h"

#import <CoreMIDI/CoreMIDI.h>
#import <Foundation/Foundation.h>

#include <set>

namespace EP {

namespace detail {

void midiClientCallback(const MIDINotification* notification, void* __nullable refCon)
{
    if (notification->messageID == kMIDIMsgSetupChanged)
    {
        auto* midiManager = static_cast<MidiManager*>(refCon);
        if (!midiManager) return;

        // Reschedule to main thread using GCD
        dispatch_async(dispatch_get_main_queue(), ^{
            if (midiManager)
            {
                midiManager->handleMIDIConfigChanged();
            }
        });
    }
}

void midiPortCallback(const MIDIEventList* evtList, void* __nullable refCon)
{
    const MIDIEventPacket* packet = &evtList->packet[0];
    for (UInt32 i = 0; i < evtList->numPackets; ++i)
    {
        if (packet->wordCount >= 1)
        {
            uint32_t word = packet->words[0];

            // UMP format for MIDI 1.0 channel voice messages:
            // [messageType:4][group:4][status:8][data1:8][data2:8]
            uint8_t messageType = (word >> 28) & 0x0F;

            if (messageType == 0x02)
            {
                // MIDI 1.0 channel voice message
                uint8_t midiStatus = (word >> 16) & 0xFF;
                uint8_t data1      = (word >> 8) & 0xFF;
                uint8_t data2      = word & 0xFF;

                if (auto* midiManager = static_cast<MidiManager*>(refCon))
                {
                    midiManager->handleMIDIMessage(midiStatus, data1, data2);
                }
            }
        }

        packet = MIDIEventPacketNext(packet);
    }
}

} // namespace detail

struct CoreMidiManagerData
{
    MIDIClientRef             client = 0;
    MIDIPortRef               inputPort = 0;
    std::set<MIDIEndpointRef> connectedSources;
};

CoreMidiManager::CoreMidiManager(EventCallback eventCB, ConfigChangedCallback configCB, const std::string& mappingsPath)
    : MidiManager(std::move(eventCB), std::move(configCB), mappingsPath)
    , m_data(std::make_unique<CoreMidiManagerData>())
{
    // Create MIDI client
    OSStatus status = MIDIClientCreate(
        CFSTR("EPSamplePlayer"),
        detail::midiClientCallback,
        static_cast<MidiManager*>(this),
        &m_data->client
    );

    if (status != noErr)
    {
        throw new std::runtime_error("CoreMidiManager: Failed to create MIDI client!");
    }

    // Create input port with MIDI 1.0 protocol
    // The receive block runs on a CoreMIDI real-time thread
    status = MIDIInputPortCreateWithProtocol(
        m_data->client,
        CFSTR("EPSamplePlayer Input"),
        kMIDIProtocol_1_0,
        &m_data->inputPort,
        ^(const MIDIEventList* evtList, void* __nullable srcConnRefCon) {
          detail::midiPortCallback(evtList, srcConnRefCon);
        }
    );

    if (status != noErr)
    {
        MIDIClientDispose(m_data->client);
        throw new std::runtime_error("CoreMidiManager: Failed to create MIDI input port!");
    }
}

CoreMidiManager::~CoreMidiManager()
{
    if (m_data)
    {
        // Disconnect all connected sources
        for (auto& src : m_data->connectedSources)
        {
            MIDIPortDisconnectSource(m_data->inputPort, src);
        }
        m_data->connectedSources.clear();

        if (m_data->inputPort)
        {
            MIDIPortDispose(m_data->inputPort);
        }

        if (m_data->client)
        {
            MIDIClientDispose(m_data->client);
        }
    }
}

int CoreMidiManager::sourceCount() const
{
    return static_cast<int>(MIDIGetNumberOfSources());
}

std::string CoreMidiManager::sourceName(int index) const
{
    if (index >= 0 && index < sourceCount())
    {
      CFStringRef name = nullptr;
      const MIDIEndpointRef source = MIDIGetSource(index);
      OSStatus status = MIDIObjectGetStringProperty(source, kMIDIPropertyDisplayName, &name);

      if (status != noErr || !name)
      {
          return "Unknown MIDI Source";
      }

      const CFIndex length = CFStringGetMaximumSizeForEncoding(CFStringGetLength(name), kCFStringEncodingUTF8) + 1;
      char* cstr = static_cast<char*>(malloc(length));
      if (!CFStringGetCString(name, cstr, length, kCFStringEncodingUTF8))
      {
          CFRelease(name);
          free(cstr);
          return "Unknown MIDI Source";
      }

      CFRelease(name);
      if (!cstr) return {};
      std::string result(cstr);
      free(cstr);
      return result;
    }

    return {};
}

bool CoreMidiManager::connectSource(int index)
{
    if (m_data)
    {
        if (index >= 0 && index < sourceCount())
        {
            const MIDIEndpointRef source = MIDIGetSource(index);
            const OSStatus status = MIDIPortConnectSource(m_data->inputPort, source, nullptr);

            if (status != noErr)
            {
                NSLog(@"CoreMidiManager: Failed to connect source %d: %d", index, (int)status);
                return false;
            }

            m_data->connectedSources.insert(source);
            return true;
        }
    }

    return false;
}

bool CoreMidiManager::disconnectSource(int index)
{
    if (m_data)
    {
        if (index >= 0 && index < sourceCount())
        {
          const MIDIEndpointRef source = MIDIGetSource(index);
          const OSStatus status = MIDIPortDisconnectSource(m_data->inputPort, source);

          if (status != noErr)
          {
              NSLog(@"CoreMidiManager: Failed to disconnect source %d: %d", index, (int)status);
              return false;
          }

          m_data->connectedSources.erase(source);
          return true;
        }
    }

    return false;
}

bool CoreMidiManager::isSourceConnected(int index) const
{
    if (m_data)
    {
        if (index >= 0 && index < sourceCount())
        {
            const MIDIEndpointRef source = MIDIGetSource(index);
            return m_data->connectedSources.count(source) > 0;
        }
    }

    return false;
}

} // namespace EP
