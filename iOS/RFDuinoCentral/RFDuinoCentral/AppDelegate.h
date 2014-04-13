//////////////////
//
// Prototype central for the rfduino
//
#import <UIKit/UIKit.h>
#import "BluetoothLE.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
    BluetoothLE *m_ble;
}

@property (strong, nonatomic) UIWindow *window;

@end
