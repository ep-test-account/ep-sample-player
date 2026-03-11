#import "SamplePlayerBridge.h"

#include "SamplePlayerCore.hpp"

#import <AppKit/AppKit.h>

#include <memory>
#include <string>
#include <vector>

namespace detail {

/// Resolves the MIDI mapping file path.
/// Returns the user-override path if it exists, otherwise the bundle path.
/// Returns an empty string if neither is found.
std::string resolveMidiMappingPath()
{
    // 1. User override: ~/Library/Application Support/com.petertools.EP-Sample-Player/midi_mapping.json
    NSString *bundleID = [[NSBundle mainBundle] bundleIdentifier];
    NSArray<NSString *> *appSupportDirs =
        NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);

    if (appSupportDirs.count > 0 && bundleID)
    {
        NSString *userPath = [[appSupportDirs.firstObject
                               stringByAppendingPathComponent:bundleID]
                              stringByAppendingPathComponent:@"midi_mapping.json"];

        if ([[NSFileManager defaultManager] fileExistsAtPath:userPath])
            return std::string(userPath.UTF8String);
    }

    // 2. Bundle default
    NSString *bundlePath = [[NSBundle mainBundle] pathForResource:@"midi_mapping" ofType:@"json"];
    if (bundlePath)
        return std::string(bundlePath.UTF8String);

    return {};
}

/// Shows a modal alert on the main thread describing a mapping load failure.
void showMappingAlert(NSString *detail)
{
    NSAlert *alert      = [[NSAlert alloc] init];
    alert.alertStyle    = NSAlertStyleWarning;
    alert.messageText   = @"MIDI Mapping Error";
    alert.informativeText = [NSString stringWithFormat:
        @"The MIDI mapping file could not be loaded. MIDI triggers will be disabled.\n\n%@",
        detail];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

} // namespace detail

// ─── Bridge ──────────────────────────────────────────────────────────────────

@implementation SamplePlayerBridge {
    std::unique_ptr<EP::SamplePlayerCore> _core;
}

- (instancetype)init {
    self = [super init];

    if (self)
    {
        // ── Resolve sample paths ────────────────────────────────────────────
        std::vector<std::string> samplePaths;
        for (int i = 0; i < EP::kNumPads; ++i)
        {
            NSString *name = [NSString stringWithFormat:@"pad_%02d", i + 1];
            NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav" inDirectory:@"Samples"];

            if (!path)
                path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];

            samplePaths.push_back(path ? std::string(path.UTF8String) : std::string());
        }

        // ── Wire C++ callbacks to Obj-C delegate ────────────────────────────
        __weak typeof(self) weakSelf = self;

        const auto midiConfigCB = [weakSelf] {
                __strong typeof(weakSelf) strongSelf = weakSelf;
                if (strongSelf &&
                    [strongSelf.delegate respondsToSelector:@selector(midiConfigurationDidChange)]) {
                    [strongSelf.delegate midiConfigurationDidChange];
                }
            };

        const auto padStateCB = [weakSelf] (int padIndex, bool isPlaying) {
                __strong typeof(weakSelf) strongSelf = weakSelf;
                if (strongSelf && strongSelf.delegate) {
                    [strongSelf.delegate padStateDidChange:padIndex isPlaying:isPlaying];
                }
            };

        try
        {
            const std::string mappingsPath = detail::resolveMidiMappingPath();

            _core = std::make_unique<EP::SamplePlayerCore>(
                samplePaths, mappingsPath, midiConfigCB, padStateCB);
        }
        catch (const std::exception& err)
        {
            dispatch_async(dispatch_get_main_queue(), ^{
              detail::showMappingAlert([NSString stringWithUTF8String: err.what()]);
            });
        }
    }

    return self;
}

- (void)dealloc {
    _core.reset();
}

// ─── Forwarding ─────────────────────────────────────────────────────────────

- (void)start {
    _core->start();
}

- (void)stop {
    _core->stop();
}

- (void)togglePad:(int)padIndex {
    _core->togglePad(padIndex);
}

- (void)setPadVolume:(float)volume forPad:(int)padIndex {
    _core->setPadVolume(padIndex, volume);
}

- (void)setMasterVolume:(float)masterVolume {
    _masterVolume = masterVolume;
    _core->setMasterVolume(masterVolume);
}

- (void)setFadeEnabled:(BOOL)enabled forPad:(int)padIndex {
    _core->setFadeEnabled(padIndex, enabled);
}

- (BOOL)isPadPlaying:(int)padIndex {
    return _core->isPadPlaying(padIndex) ? YES : NO;
}

- (int)numberOfPads {
    return _core->numberOfPads();
}

// ─── MIDI mapping ────────────────────────────────────────────────────────────

- (NSString *)midiLabelForPad:(int)padIndex {
    const auto label = _core->getPadMappingLabel(padIndex);
    if (label.empty()) return nil;
    return [NSString stringWithUTF8String:label.c_str()];
}

// ─── MIDI device management ─────────────────────────────────────────────────

- (int)midiSourceCount {
    return _core->midiSourceCount();
}

- (nullable NSString *)midiSourceNameAtIndex:(int)index {
    const auto name = _core->midiSourceName(index);
    if (name.empty()) return nil;
    return [NSString stringWithUTF8String:name.c_str()];
}

- (BOOL)connectMIDISource:(int)index {
    return _core->connectMIDISource(index) ? YES : NO;
}

- (BOOL)disconnectMIDISource:(int)index {
    return _core->disconnectMIDISource(index) ? YES : NO;
}

- (BOOL)isMIDISourceConnected:(int)index {
    return _core->isMIDISourceConnected(index) ? YES : NO;
}

@end
