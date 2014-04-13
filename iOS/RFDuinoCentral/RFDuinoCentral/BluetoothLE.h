/////////////////////////
//
// Bluetooth connection manager. Host acts as the central device, manages connections with peripherals
// and routes messages to/from the peripherals accordingly. For each peripheral, an associated event handling
// object is created. "Programs" are loaded into these event handlers.
//
#pragma once
#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import "BLEDelegate.h" 

#define DISCONNECTED    0
#define SCANNING        1
#define CONNECTED       2


@interface BluetoothLE : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
    CBCentralManager *m_manager;
        
    CBCharacteristic *m_send_characteristic;
    CBCharacteristic *m_disconnect_characteristic;
    CBPeripheral *m_peripheral;
    
    bool m_bleEnabled;
    short m_connectionState;
    
    NSTimer *m_chunkTimer;
    
    uint8_t *m_chunkData;
    NSString *m_chunkDataName;
    int m_chunkDataSize;
    int m_curChunkIndex;
    
    int m_chunkState;
}

@property(assign, nonatomic) id<BLEDelegate> delegate;

-(void) startScan;
-(void) disconnectPeripherals;
-(short) connectionState;
-(void) sendData: (NSData *)data;
-(bool) sendDataChunk: (NSData *)data withName:(NSString *)name;

@end
