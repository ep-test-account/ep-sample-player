#import <Cocoa/Cocoa.h>

@class SamplePlayerBridge;

NS_ASSUME_NONNULL_BEGIN

/// Window controller for the MIDI device preferences.
/// Displays a list of available MIDI sources with connect/disconnect buttons.
@interface PreferencesWindowController : NSWindowController

- (instancetype)initWithBridge:(SamplePlayerBridge *)bridge;

@end

NS_ASSUME_NONNULL_END
