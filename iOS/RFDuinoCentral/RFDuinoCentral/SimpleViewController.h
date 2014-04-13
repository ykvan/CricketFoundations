//////////////////
//
// Prototype central. Send wav data to the rfduino for playback.
//
#import "BluetoothLE.h"
#import "BLEDelegate.h"

@interface SimpleViewController : UIViewController <BLEDelegate> {
    UILabel *m_label;
    UIButton *m_sendButton;
    UIButton *m_playButton;
    UIButton *m_bleButton;
    
    BluetoothLE *m_ble;
}

-(id) initWithBle: (BluetoothLE *)ble;

@end