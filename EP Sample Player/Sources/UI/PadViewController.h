#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/// Main view controller managing the 1x8 pad grid, per-pad volume sliders,
/// and master volume control.
///
/// Retrieves its SamplePlayerBridge from the AppDelegate.
@interface PadViewController : NSViewController

@end

NS_ASSUME_NONNULL_END
