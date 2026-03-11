#import "PadViewController.h"
#import "PadView.h"
#import "SamplePlayerBridge.h"
#import "AppDelegate.h"

@interface PadViewController () <SamplePlayerBridgeDelegate>

@property (nonatomic, strong) SamplePlayerBridge *bridge;
@property (nonatomic, strong) NSMutableArray<PadView *> *padViews;
@property (nonatomic, strong) NSMutableArray<NSSlider *> *volumeSliders;
@property (nonatomic, strong) NSSlider *masterVolumeSlider;
@property (nonatomic, strong) NSTextField *masterVolumeLabel;
@property (nonatomic, strong) NSTextField *midiInfoLabel;

@end

@implementation PadViewController

- (void)loadView {
    self.view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 900, 300)];
    self.view.wantsLayer = YES;
    self.view.layer.backgroundColor = [NSColor colorWithCalibratedWhite:0.15 alpha:1.0].CGColor;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Get the bridge from the AppDelegate
    AppDelegate *appDelegate = (AppDelegate *)[NSApp delegate];
    self.bridge = appDelegate.bridge;

    self.padViews = [NSMutableArray array];
    self.volumeSliders = [NSMutableArray array];

    self.bridge.delegate = self;

    [self setupMasterVolumeControl];
    [self setupPadGrid];
    [self setupMIDIInfoLabel];
}

// ─── UI Setup ────────────────────────────────────────────────────────────────

- (void)setupMasterVolumeControl {
    // Master volume label
    _masterVolumeLabel = [NSTextField labelWithString:@"Master Volume"];
    _masterVolumeLabel.font = [NSFont systemFontOfSize:12.0 weight:NSFontWeightMedium];
    _masterVolumeLabel.textColor = [NSColor colorWithCalibratedWhite:0.8 alpha:1.0];
    _masterVolumeLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_masterVolumeLabel];

    // Master volume slider
    _masterVolumeSlider = [[NSSlider alloc] init];
    _masterVolumeSlider.minValue = 0.0;
    _masterVolumeSlider.maxValue = 1.0;
    _masterVolumeSlider.doubleValue = 1.0;
    _masterVolumeSlider.target = self;
    _masterVolumeSlider.action = @selector(masterVolumeChanged:);
    _masterVolumeSlider.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_masterVolumeSlider];

    [NSLayoutConstraint activateConstraints:@[
        [_masterVolumeLabel.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:12],
        [_masterVolumeLabel.trailingAnchor constraintEqualToAnchor:_masterVolumeSlider.leadingAnchor constant:-8],

        [_masterVolumeSlider.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:10],
        [_masterVolumeSlider.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-16],
        [_masterVolumeSlider.widthAnchor constraintEqualToConstant:150],
    ]];
}

- (void)setupPadGrid {
    int numPads = self.bridge.numberOfPads;

    // Container view for the pad grid
    NSView *gridContainer = [[NSView alloc] init];
    gridContainer.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:gridContainer];

    [NSLayoutConstraint activateConstraints:@[
        [gridContainer.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:16],
        [gridContainer.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-16],
        [gridContainer.topAnchor constraintEqualToAnchor:self.view.topAnchor constant:40],
        [gridContainer.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-40],
    ]];

    // Create a horizontal stack of pads with volume sliders
    NSView *previousColumn = nil;
    CGFloat padSpacing = 8.0;

    for (int i = 0; i < numPads; ++i) {
        // Column container for pad + slider
        NSView *column = [[NSView alloc] init];
        column.translatesAutoresizingMaskIntoConstraints = NO;
        [gridContainer addSubview:column];

        // Pad view
        PadView *padView = [[PadView alloc] initWithFrame:NSZeroRect];
        padView.noteName = [self.bridge midiLabelForPad:i];
        padView.padIndex = i;
        padView.translatesAutoresizingMaskIntoConstraints = NO;

        __weak typeof(self) weakSelf = self;
        padView.onToggle = ^{
            [weakSelf.bridge togglePad:i];
        };

        [column addSubview:padView];
        [self.padViews addObject:padView];

        // Volume slider (vertical)
        NSSlider *volumeSlider = [[NSSlider alloc] init];
        volumeSlider.sliderType = NSSliderTypeLinear;
        volumeSlider.vertical = YES;
        volumeSlider.minValue = 0.0;
        volumeSlider.maxValue = 1.0;
        volumeSlider.doubleValue = 1.0;
        volumeSlider.tag = i;
        volumeSlider.target = self;
        volumeSlider.action = @selector(padVolumeChanged:);
        volumeSlider.translatesAutoresizingMaskIntoConstraints = NO;
        [column addSubview:volumeSlider];
        [self.volumeSliders addObject:volumeSlider];

        // Layout within column: pad on top, slider below
        [NSLayoutConstraint activateConstraints:@[
            [padView.topAnchor constraintEqualToAnchor:column.topAnchor],
            [padView.leadingAnchor constraintEqualToAnchor:column.leadingAnchor],
            [padView.trailingAnchor constraintEqualToAnchor:column.trailingAnchor],
            [padView.heightAnchor constraintEqualToAnchor:padView.widthAnchor], // square

            [volumeSlider.topAnchor constraintEqualToAnchor:padView.bottomAnchor constant:8],
            [volumeSlider.centerXAnchor constraintEqualToAnchor:column.centerXAnchor],
            [volumeSlider.bottomAnchor constraintEqualToAnchor:column.bottomAnchor],
            [volumeSlider.heightAnchor constraintGreaterThanOrEqualToConstant:60],
        ]];

        // Horizontal layout: columns side by side with equal widths
        [NSLayoutConstraint activateConstraints:@[
            [column.topAnchor constraintEqualToAnchor:gridContainer.topAnchor],
            [column.bottomAnchor constraintEqualToAnchor:gridContainer.bottomAnchor],
        ]];

        if (previousColumn) {
            [NSLayoutConstraint activateConstraints:@[
                [column.leadingAnchor constraintEqualToAnchor:previousColumn.trailingAnchor constant:padSpacing],
                [column.widthAnchor constraintEqualToAnchor:previousColumn.widthAnchor],
            ]];
        } else {
            [column.leadingAnchor constraintEqualToAnchor:gridContainer.leadingAnchor].active = YES;
        }

        if (i == numPads - 1) {
            [column.trailingAnchor constraintEqualToAnchor:gridContainer.trailingAnchor].active = YES;
        }

        previousColumn = column;
    }
}

- (void)setupMIDIInfoLabel {
    _midiInfoLabel = [NSTextField labelWithString:@"MIDI: loaded from midi_mapping.json"];
    _midiInfoLabel.font = [NSFont systemFontOfSize:11.0];
    _midiInfoLabel.textColor = [NSColor colorWithCalibratedWhite:0.6 alpha:1.0];
    _midiInfoLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:_midiInfoLabel];

    [NSLayoutConstraint activateConstraints:@[
        [_midiInfoLabel.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:16],
        [_midiInfoLabel.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-10],
    ]];
}

// ─── Actions ─────────────────────────────────────────────────────────────────

- (void)masterVolumeChanged:(NSSlider *)sender {
    self.bridge.masterVolume = (float)sender.doubleValue;
}

- (void)padVolumeChanged:(NSSlider *)sender {
    int padIndex = (int)sender.tag;
    [self.bridge setPadVolume:(float)sender.doubleValue forPad:padIndex];
}

// ─── SamplePlayerBridgeDelegate ──────────────────────────────────────────────

- (void)padStateDidChange:(int)padIndex isPlaying:(BOOL)isPlaying {
    if (padIndex >= 0 && padIndex < (int)self.padViews.count) {
        self.padViews[padIndex].playing = isPlaying;
    }
}

@end
