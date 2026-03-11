#import <Foundation/Foundation.h>

/// Delegate protocol for discrete engine state change events.
@protocol SamplePlayerBridgeDelegate <NSObject>

/// Called on the main thread when a pad's playing state changes.
- (void)padStateDidChange:(int)padIndex isPlaying:(BOOL)isPlaying;

/// Called on the main thread when MIDI device configuration changes.
- (void)midiConfigurationDidChange;

@end

/// Thin Objective-C wrapper around the pure C++ SamplePlayerCore.
///
/// This class resolves bundle paths and forwards all calls to the C++ core.
/// It exists solely to provide an Obj-C interface for the AppKit UI layer.
@interface SamplePlayerBridge : NSObject

/// Delegate for discrete state change events.
@property (nonatomic, weak, nullable) id<SamplePlayerBridgeDelegate> delegate;

/// Number of pads (read-only).
@property (nonatomic, readonly) int numberOfPads;

/// Master volume (0.0 – 1.0).
@property (nonatomic) float masterVolume;

/// Initialize the bridge: creates C++ core, loads samples from bundle, sets up MIDI.
- (instancetype _Nonnull)init;

/// Returns a short display label for the MIDI trigger assigned to a pad,
/// e.g. @"C1", @"D#2", @"CC 20". Returns @"—" if the pad has no mapping.
- (NSString * _Nonnull)midiLabelForPad:(int)padIndex;

/// Start audio output and begin processing.
- (void)start;

/// Stop audio output.
- (void)stop;

/// Toggle a pad's playback state (sent via command ring buffer).
- (void)togglePad:(int)padIndex;

/// Set volume for a specific pad (sent via command ring buffer).
- (void)setPadVolume:(float)volume forPad:(int)padIndex;

/// Set whether fade in/out is enabled for a pad (sent via command ring buffer).
- (void)setFadeEnabled:(BOOL)enabled forPad:(int)padIndex;

/// Query if a pad is currently playing (reads atomic state from engine).
- (BOOL)isPadPlaying:(int)padIndex;

// ─── MIDI device management ──────────────────────────────────────────────

/// Number of available MIDI sources.
- (int)midiSourceCount;

/// Display name of a MIDI source at the given index. Returns nil if out of range.
- (nullable NSString *)midiSourceNameAtIndex:(int)index;

/// Connect a MIDI source by index.
- (BOOL)connectMIDISource:(int)index;

/// Disconnect a MIDI source by index.
- (BOOL)disconnectMIDISource:(int)index;

/// Check if a MIDI source is connected.
- (BOOL)isMIDISourceConnected:(int)index;

@end
