#import "AppDelegate.h"

#import "SamplePlayerBridge.h"
#import "PreferencesWindowController.h"

@implementation AppDelegate

- (instancetype)init {
    self = [super init];

    if (self) {
        // Create the bridge early so it's available when the storyboard instantiates
        // PadViewController (this happens way before applicationDidFinishLaunching).
        _bridge = [[SamplePlayerBridge alloc] init];
    }

    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [_bridge start];
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    [_bridge stop];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (IBAction)showPreferences:(id)sender {
    if (!_preferencesController) {
        _preferencesController = [[PreferencesWindowController alloc] initWithBridge:_bridge];
    }

    [_preferencesController showWindow:sender];
    [_preferencesController.window makeKeyAndOrderFront:sender];
}

@end
