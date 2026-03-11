#import "PreferencesWindowController.h"
#import "SamplePlayerBridge.h"

@interface PreferencesWindowController () <NSTableViewDataSource, NSTableViewDelegate, SamplePlayerBridgeDelegate>

@property (nonatomic, strong) SamplePlayerBridge *bridge;
@property (nonatomic, strong) NSTableView *tableView;
@property (nonatomic, strong) NSButton *refreshButton;

@end

@implementation PreferencesWindowController

- (instancetype)initWithBridge:(SamplePlayerBridge *)bridge {
    NSRect frame = NSMakeRect(0, 0, 400, 300);
    NSWindowStyleMask style = NSWindowStyleMaskTitled
                            | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskResizable;

    NSWindow *window = [[NSWindow alloc] initWithContentRect:frame
                                                   styleMask:style
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
    window.title = @"MIDI Preferences";
    window.minSize = NSMakeSize(300, 200);
    [window center];

    self = [super initWithWindow:window];
    if (self) {
        _bridge = bridge;
        [self setupUI];
    }
    return self;
}

- (void)setupUI {
    NSView *contentView = self.window.contentView;
    contentView.wantsLayer = YES;

    // Title label
    NSTextField *titleLabel = [NSTextField labelWithString:@"MIDI Sources"];
    titleLabel.font = [NSFont systemFontOfSize:14.0 weight:NSFontWeightSemibold];
    titleLabel.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:titleLabel];

    // Scroll view containing the table
    NSScrollView *scrollView = [[NSScrollView alloc] init];
    scrollView.hasVerticalScroller = YES;
    scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    scrollView.borderType = NSBezelBorder;
    [contentView addSubview:scrollView];

    // Table view
    _tableView = [[NSTableView alloc] init];
    _tableView.dataSource = self;
    _tableView.delegate = self;
    _tableView.rowHeight = 30;

    NSTableColumn *nameColumn = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    nameColumn.title = @"Device";
    nameColumn.width = 250;
    [_tableView addTableColumn:nameColumn];

    NSTableColumn *statusColumn = [[NSTableColumn alloc] initWithIdentifier:@"status"];
    statusColumn.title = @"Status";
    statusColumn.width = 100;
    [_tableView addTableColumn:statusColumn];

    scrollView.documentView = _tableView;

    // Refresh button
    _refreshButton = [[NSButton alloc] init];
    _refreshButton.title = @"Refresh";
    _refreshButton.bezelStyle = NSBezelStyleRounded;
    _refreshButton.target = self;
    _refreshButton.action = @selector(refreshDevices:);
    _refreshButton.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:_refreshButton];

    // Layout
    [NSLayoutConstraint activateConstraints:@[
        [titleLabel.topAnchor constraintEqualToAnchor:contentView.topAnchor constant:12],
        [titleLabel.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:12],

        [scrollView.topAnchor constraintEqualToAnchor:titleLabel.bottomAnchor constant:8],
        [scrollView.leadingAnchor constraintEqualToAnchor:contentView.leadingAnchor constant:12],
        [scrollView.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-12],
        [scrollView.bottomAnchor constraintEqualToAnchor:_refreshButton.topAnchor constant:-8],

        [_refreshButton.trailingAnchor constraintEqualToAnchor:contentView.trailingAnchor constant:-12],
        [_refreshButton.bottomAnchor constraintEqualToAnchor:contentView.bottomAnchor constant:-12],
    ]];
}

- (void)showWindow:(id)sender {
    [super showWindow:sender];
    [_tableView reloadData];
}

// ─── Actions ─────────────────────────────────────────────────────────────────

- (void)refreshDevices:(id)sender {
    [_tableView reloadData];
}

- (void)toggleConnection:(NSButton *)sender {
    int sourceIndex = (int)sender.tag;
    BOOL isConnected = [self.bridge isMIDISourceConnected:sourceIndex];

    if (isConnected) {
        [self.bridge disconnectMIDISource:sourceIndex];
    } else {
        [self.bridge connectMIDISource:sourceIndex];
    }

    [_tableView reloadData];
}

// ─── NSTableViewDataSource ───────────────────────────────────────────────────

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [self.bridge midiSourceCount];
}

// ─── NSTableViewDelegate ─────────────────────────────────────────────────────

- (NSView *)tableView:(NSTableView *)tableView
    viewForTableColumn:(NSTableColumn *)tableColumn
                   row:(NSInteger)row
{
    int sourceIndex = (int)row;

    if ([tableColumn.identifier isEqualToString:@"name"]) {
        NSTextField *textField = [tableView makeViewWithIdentifier:@"nameCell" owner:self];
        if (!textField) {
            textField = [NSTextField labelWithString:@""];
            textField.identifier = @"nameCell";
        }
        NSString *name = [self.bridge midiSourceNameAtIndex:sourceIndex];
        textField.stringValue = name ?: @"Unknown";
        return textField;
    }

    if ([tableColumn.identifier isEqualToString:@"status"]) {
        NSButton *button = [tableView makeViewWithIdentifier:@"statusCell" owner:self];
        if (!button) {
            button = [[NSButton alloc] init];
            button.identifier = @"statusCell";
            button.bezelStyle = NSBezelStyleRounded;
            button.target = self;
            button.action = @selector(toggleConnection:);
        }

        BOOL connected = [self.bridge isMIDISourceConnected:sourceIndex];
        button.title = connected ? @"Disconnect" : @"Connect";
        button.tag = sourceIndex;
        return button;
    }

    return nil;
}

// ─── SamplePlayerBridgeDelegate (for MIDI config changes) ────────────────────

- (void)padStateDidChange:(int)padIndex isPlaying:(BOOL)isPlaying {
    // Not relevant for preferences
}

- (void)midiConfigurationDidChange {
    [_tableView reloadData];
}

@end
