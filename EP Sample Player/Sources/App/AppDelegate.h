#import <Cocoa/Cocoa.h>

// Forward declaration
@class SamplePlayerBridge;
@class PreferencesWindowController;

// App delegate
@interface AppDelegate : NSObject <NSApplicationDelegate>

@property (nonatomic, strong, readonly) SamplePlayerBridge *bridge;
@property (nonatomic, strong) PreferencesWindowController *preferencesController;

@end
