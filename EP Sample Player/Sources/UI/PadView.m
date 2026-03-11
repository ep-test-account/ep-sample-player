#import "PadView.h"

@implementation PadView

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _noteName = @"";
        _playing = NO;
        _padIndex = 0;
        self.wantsLayer = YES;
        self.layer.cornerRadius = 8.0;
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    // Background color based on playing state
    NSColor *bgColor;
    if (self.playing) {
        bgColor = [NSColor colorWithCalibratedRed:0.2 green:0.8 blue:0.4 alpha:0.8];
    } else {
        bgColor = [NSColor colorWithCalibratedWhite:0.3 alpha:0.8];
    }

    // Draw rounded rectangle background
    NSBezierPath *bgPath = [NSBezierPath bezierPathWithRoundedRect:self.bounds
                                                           xRadius:8.0
                                                           yRadius:8.0];
    [bgColor setFill];
    [bgPath fill];

    // Draw border
    NSColor *borderColor = self.playing
        ? [NSColor colorWithCalibratedRed:0.1 green:0.6 blue:0.3 alpha:1.0]
        : [NSColor colorWithCalibratedWhite:0.5 alpha:1.0];
    [borderColor setStroke];
    bgPath.lineWidth = 2.0;
    [bgPath stroke];

    // Draw note name
    NSMutableParagraphStyle *paraStyle = [[NSMutableParagraphStyle alloc] init];
    paraStyle.alignment = NSTextAlignmentCenter;

    NSDictionary *nameAttrs = @{
        NSFontAttributeName: [NSFont boldSystemFontOfSize:16.0],
        NSForegroundColorAttributeName: [NSColor whiteColor],
        NSParagraphStyleAttributeName: paraStyle,
    };

    NSSize nameSize = [self.noteName sizeWithAttributes:nameAttrs];
    NSRect nameRect = NSMakeRect(0,
                                  (self.bounds.size.height - nameSize.height) / 2.0 + 10.0,
                                  self.bounds.size.width,
                                  nameSize.height);
    [self.noteName drawInRect:nameRect withAttributes:nameAttrs];

    // Draw state label
    NSString *stateText = self.playing ? @"Playing" : @"Stopped";
    NSDictionary *stateAttrs = @{
        NSFontAttributeName: [NSFont systemFontOfSize:10.0],
        NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.9 alpha:0.8],
        NSParagraphStyleAttributeName: paraStyle,
    };

    NSSize stateSize = [stateText sizeWithAttributes:stateAttrs];
    NSRect stateRect = NSMakeRect(0,
                                   (self.bounds.size.height - stateSize.height) / 2.0 - 12.0,
                                   self.bounds.size.width,
                                   stateSize.height);
    [stateText drawInRect:stateRect withAttributes:stateAttrs];
}

- (void)setPlaying:(BOOL)playing {
    if (_playing != playing) {
        _playing = playing;
        [self setNeedsDisplay:YES];
    }
}

- (void)mouseDown:(NSEvent *)event {
    if (self.onToggle) {
        self.onToggle();
    }
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event {
    return YES;
}

@end
