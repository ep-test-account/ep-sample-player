#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/// Custom NSView to display a single sample pad.
/// Displays the pad's note name, playing state (color), and handles click events.
@interface PadView : NSView

/// The note name displayed on the pad
@property (nonatomic, copy) NSString *noteName;

/// Whether the pad is currently playing.
@property (nonatomic) BOOL playing;

/// The pad index (this is 0-based)
@property (nonatomic) int padIndex;

/// Block called when the pad is clicked.
@property (nonatomic, copy, nullable) void (^onToggle)(void);

@end

NS_ASSUME_NONNULL_END
