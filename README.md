# EP Sample Player

A macOS sample player with 8 trigger pads, real-time audio mixing, and flexible MIDI mapping via an external JSON file.

## Requirements

- macOS 12+
- Xcode 14+
- [nlohmann/json](https://github.com/nlohmann/json) single-header

## Building

Open `EP Sample Player.xcodeproj` in Xcode and hit Run.

## Project Structure

```
EP Sample Player/
├── Sources/
│   ├── App/                  # Application entry point and Obj-C/C++ bridge
│   ├── Core/                 # Platform-agnostic C++ engine
│   │   ├── AudioEngine       # Real-time audio mixing and voice management
│   │   ├── SampleSlot        # Single pad voice (playback, fade in/out)
│   │   ├── AudioBuffer       # Immutable PCM sample container
│   │   ├── MIDIManager       # MIDI message parsing and pad mapping
│   │   ├── MidiMappingLoader # JSON mapping file parser
│   │   ├── RingBuffer        # Lock-free SPSC queue (audio/midi ↔ UI comms)
│   │   └── Types             # Shared constants and message structs
│   ├── Platform/             # macOS-specific wrappers
│   │   ├── CoreAudioOutput   # CoreAudio HAL I/O
│   │   ├── CoreMIDIHost      # CoreMIDI source management
│   │   ├── AudioFileLoader   # Audio file decoding (WAV, MP3, AIFF…)
│   │   └── GDCTimer          # GCD-based polling timer (~30 Hz)
│   ├── UI/                   # Cocoa/AppKit interface
│   │   ├── PadViewController # Main view: 8 pads + volume sliders
│   │   ├── PadView           # Individual pad widget
│   │   └── PreferencesWindowController # MIDI device selection
│   └── Resources/
│       ├── Samples/          # Bundled WAV files (pad_01.wav … pad_08.wav)
│       └── midi_mapping.json # Default MIDI mapping (editable)
└── EP Sample Player.xcodeproj
```

## MIDI Mapping

Pad-to-MIDI assignments are defined in `midi_mapping.json`. The app loads a user override from `~/Library/Application Support/<bundle id>/midi_mapping.json` if present, falling back to the bundled default.

Each entry supports **Note On** and **Control Change** messages:

```json
{
  "mappings": [
    { "pad": 0, "type": "note", "channel": 1, "value": 36 },
    { "pad": 1, "type": "cc",   "channel": 1, "value": 20 }
  ]
}
```

| Field     | Description                          |
|-----------|--------------------------------------|
| `pad`     | Pad index, 0–7                       |
| `type`    | `"note"` or `"cc"`                   |
| `channel` | MIDI channel, 1–16                   |
| `value`   | Note number or CC number, 0–127      |

## Architecture Notes

The engine uses **lock-free ring buffers** throughout to keep the CoreAudio render thread free of locks and allocations:

- UI → Engine: command ring buffer (toggle pad, set volume, …)
- MIDI → Engine: MIDI event ring buffer
- Engine → UI: event ring buffer (pad started / stopped), polled at ~30 Hz via GCD timer
