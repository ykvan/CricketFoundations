//////////////////
//
// Prototype central. Send wav data to the rfduino for playback.
//
#import "SimpleViewController.h"

@implementation SimpleViewController

const short REQ_PLAYWAV = 0x0a;;

- (id) initWithBle: (BluetoothLE *) ble {
    self = [super init];
    self->m_ble = ble;
        
    return self;
}

- (void) loadView {
    [super loadView];
    self.view.backgroundColor = [UIColor whiteColor];

    m_bleButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [m_bleButton setTitleColor:[UIColor blueColor] forState:UIControlStateNormal];
    m_bleButton.backgroundColor = [UIColor whiteColor];
    m_bleButton.layer.borderColor = [UIColor blueColor].CGColor;
    m_bleButton.layer.borderWidth = 0.5f;
    m_bleButton.layer.cornerRadius = 10.0f;
    m_bleButton.frame = CGRectInset(CGRectMake(50,50,180,50),0, 0);
    m_bleButton.titleLabel.font = [UIFont systemFontOfSize: 30];
    m_bleButton.titleLabel.textAlignment = NSTextAlignmentLeft;
    [m_bleButton setTitle:@"Connect" forState:UIControlStateNormal];
    [m_bleButton addTarget:self action:@selector(bleButtonHandler:)
          forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:m_bleButton];

    
    m_sendButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [m_sendButton setTitleColor:[UIColor blueColor] forState:UIControlStateNormal];
    m_sendButton.backgroundColor = [UIColor whiteColor];
    m_sendButton.layer.borderColor = [UIColor blueColor].CGColor;
    m_sendButton.layer.borderWidth = 0.5f;
    m_sendButton.layer.cornerRadius = 10.0f;
    m_sendButton.frame = CGRectInset(CGRectMake(50,120,90,50),0, 0);
    m_sendButton.titleLabel.font = [UIFont systemFontOfSize: 30];
    m_sendButton.titleLabel.textAlignment = NSTextAlignmentLeft;
    [m_sendButton setTitle:@"Send" forState:UIControlStateNormal];
    [m_sendButton addTarget:self action:@selector(sendButtonHandler:)
            forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:m_sendButton];

    m_playButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [m_playButton setTitleColor:[UIColor blueColor] forState:UIControlStateNormal];
    m_playButton.backgroundColor = [UIColor whiteColor];
    m_playButton.layer.borderColor = [UIColor blueColor].CGColor;
    m_playButton.layer.borderWidth = 0.5f;
    m_playButton.layer.cornerRadius = 10.0f;
    m_playButton.frame = CGRectInset(CGRectMake(50,190,90,50),0, 0);
    m_playButton.titleLabel.font = [UIFont systemFontOfSize: 30];
    m_playButton.titleLabel.textAlignment = NSTextAlignmentLeft;
    [m_playButton setTitle:@"Play" forState:UIControlStateNormal];
    [m_playButton addTarget:self action:@selector(playButtonHandler:)
         forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:m_playButton];

    m_label = [[UILabel alloc] init];
    m_label.frame = CGRectInset(CGRectMake(50,280,220,20),0, 0);
    m_label.autoresizingMask = UIViewAutoresizingFlexibleBottomMargin+UIViewAutoresizingFlexibleWidth+UIViewAutoresizingFlexibleRightMargin;
    //m_label.backgroundColor = [UIColor redColor];
    m_label.font = [UIFont systemFontOfSize: 16];
    m_label.text = @"Disconnected...";
    [self.view addSubview:m_label];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    if(UIInterfaceOrientationPortrait == toInterfaceOrientation ||
       UIInterfaceOrientationPortraitUpsideDown == toInterfaceOrientation) {
        NSLog(@"Portrait");
    } else {
        NSLog(@"Landscape");
    }
}

//////////////////
//
// Send audio data to the rfduino
//
- (void) sendButtonHandler:(id) sender {
    // send the wav data payload (w/ simple checksum)
    NSString *path = [[NSBundle mainBundle] pathForResource: @"test" ofType: @"wav"];
    NSData *data = [NSData dataWithContentsOfFile:path];

    if([m_ble sendDataChunk: data withName:@"test.wav"] == false) {
        NSLog(@"The associated name of the data chunk needs to be 18 characters or less in length");
    }
}

//////////////////
//
// Send the play audio msg to the rfduino
//
- (void) playButtonHandler:(id) sender {
    NSLog(@"Sending play cmd...");
    NSData *pdata = [NSData dataWithBytes:(void*)&REQ_PLAYWAV length:2];
    [m_ble sendData:pdata];
    [self updateLabel:@"Play test.wav"];

}

//////////////////
//
// Sned the play audio msg to the rfduino
//
- (void) bleButtonHandler:(id) sender {
    if([self->m_ble connectionState] == DISCONNECTED)
        [self->m_ble startScan];
    else if([self->m_ble connectionState] == CONNECTED)
        [self->m_ble disconnectPeripherals];
}

- (void)updateConnectionState:(short)state {
    if(state == DISCONNECTED) {
        m_label.text = @"Disconnected...";
        [m_bleButton setTitle:@"Connect" forState:UIControlStateNormal];
    } else if(state == SCANNING) {
        m_label.text = @"Scanning...";
    } else if(state == CONNECTED) {
        m_label.text = @"Connected...";
        [m_bleButton setTitle:@"Disconnect" forState:UIControlStateNormal];
    }
}

- (void)updateLabel:(NSString *)label {
    m_label.text = label;
}


- (void)didReceive:(NSData *)data
{
    NSLog(@"didReceive");
}


@end