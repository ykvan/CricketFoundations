/////////////////////////
//
// Bluetooth connection manager. Host acts as the central device, manages connections with peripherals
// and routes messages to/from the peripherals accordingly. For each peripheral, an associated event handling
// object is created. "Programs" are loaded into these event handlers.
//
#import "BluetoothLE.h"


// our current eco system of two device types

@implementation BluetoothLE

@synthesize delegate;

static CBUUID *m_service_uuid;
static CBUUID *m_send_uuid;
static CBUUID *m_receive_uuid;
static CBUUID *m_disconnect_uuid;

#define CHUNK_DISABLED          0
#define CHUNK_SENDREQ           1
#define CHUNK_WAITINGRESP       2
#define CHUNK_SENDCHUNKS        3
#define CHUNK_FINISHEDCHUNKS    4

const short REQ_SENDCHUNKDATA        = 0x02;
const short ACK_SENDCHUNKDATA        = 0x03;
const short ACK_READYTORECVCHUNKDATA = 0x05;
const short ACK_GOODCHUNKDATA        = 0x07;
const short ACK_BADCHUNKDATA         = 0x08;

-(id)init {
    self = [super init];
    if (self) {
        
        NSLog(@"Bluethooth LE central host is running...\n");
        
        m_manager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
        
        // the rfduino service uuid
        m_service_uuid = [CBUUID UUIDWithString:@"2220"];
        
        // the rfduino characteristic uuids
        m_send_uuid = [CBUUID UUIDWithString:@"2222"];
        m_receive_uuid = [CBUUID UUIDWithString:@"2221"];
        m_disconnect_uuid = [CBUUID UUIDWithString:@"2223"];

        m_bleEnabled = false;
        m_connectionState = DISCONNECTED;
        
        m_chunkData = nil;
        m_chunkState = CHUNK_DISABLED;
    }
    return self;
}

//////////////////////
//
// State change on the central manager (i.e startup)
//
- (void) centralManagerDidUpdateState:(CBCentralManager *)central {
    if([self isLECapableHardware]) {
        m_bleEnabled = true;
    }
}

//////////////////////
//
// Check whether the device is enabled for Bluetooth LE
//
- (BOOL) isLECapableHardware {
    NSString * state = nil;
    
    switch ([m_manager state])
    {
        case CBCentralManagerStateUnsupported:
            state = @"The platform/hardware doesn't support Bluetooth Low Energy.";
            break;
        case CBCentralManagerStateUnauthorized:
            state = @"The app is not authorized to use Bluetooth Low Energy.";
            break;
        case CBCentralManagerStatePoweredOff:
            state = @"Bluetooth is currently powered off.";
            break;
        case CBCentralManagerStatePoweredOn:
            return TRUE;
        case CBCentralManagerStateUnknown:
        default:
            return FALSE;
    }
    
    NSLog(@"Failure: %@", state);
    return FALSE;
}

//////////////////////
//
// Scan for peripherals that have the services we are interested in....
//
- (void) startScan {
    if(m_bleEnabled) {
        NSDictionary *options = nil;
        options = [NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES]
                                          forKey:CBCentralManagerScanOptionAllowDuplicatesKey];
        [m_manager scanForPeripheralsWithServices:[NSArray arrayWithObject:m_service_uuid] options:options];
        
        [self updateConnectionState: SCANNING];
    }
}

//////////////////////
//
// Update the connection state
//
- (void) updateConnectionState: (short)state {
    m_connectionState = state;
    [delegate updateConnectionState:m_connectionState];
}

//////////////////////
//
// Stop scanning for peripherals
//
- (void) stopScan {
    [m_manager stopScan];
}

//////////////////////
//
// Handler for discovered rfduino devices
//
- (void) centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)aPeripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI {
    m_peripheral = aPeripheral;
    [m_manager connectPeripheral:aPeripheral options:nil];
}

//////////////////////
//
// On connect, discover its services and route notifications for the connect peripheral
//
- (void) centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)aPeripheral {
    [aPeripheral setDelegate:self];
    [aPeripheral discoverServices:nil];
}

//////////////////////
//
// On disconnect, cleanup
//
- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error {
    [aPeripheral setDelegate:nil];
    m_peripheral = nil;
    [self updateConnectionState: DISCONNECTED];
}

//////////////////////
//
// Failuer handler
//
- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)aPeripheral error:(NSError *)error {
    NSLog(@"Fail to connect to peripheral: %@ with error = %@", aPeripheral, [error localizedDescription]);
}

//////////////////////
//
// Discover the services
//
- (void) peripheral:(CBPeripheral *)aPeripheral didDiscoverServices:(NSError *)error {
    if(error != nil) {
        printf("Bad discovery of services...\n");
        [m_manager cancelPeripheralConnection:aPeripheral];
        
        return;
    }
    NSLog(@"didDiscoverServices");
    
    for (CBService *service in aPeripheral.services) {
        if ([service.UUID isEqual:m_service_uuid])
        {
            NSArray *characteristics = [NSArray arrayWithObjects:m_receive_uuid, m_send_uuid, m_disconnect_uuid, nil];
            [aPeripheral discoverCharacteristics:characteristics forService:service];
        }
    }
}

//////////////////////
//
// Handle the characteristics
//
- (void) peripheral:(CBPeripheral *)aPeripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
    NSLog(@"didDiscoverCharacteristicsForService");
    for (CBService *service in aPeripheral.services) {
        if ([service.UUID isEqual:m_service_uuid]) {
            for (CBCharacteristic *characteristic in service.characteristics) {
                if ([characteristic.UUID isEqual:m_receive_uuid]) {
                    [aPeripheral setNotifyValue:YES forCharacteristic:characteristic];
                } else if ([characteristic.UUID isEqual:m_send_uuid]) {
                    m_send_characteristic = characteristic;
                } else if ([characteristic.UUID isEqual:m_disconnect_uuid]) {
                    m_disconnect_characteristic = characteristic;
                }
            }
            
            [self updateConnectionState: CONNECTED];
            [self stopScan];
        }
    }
}

//////////////////
//
// Handler for a notification/indication coming from a central read request or a send from the peripheral
//
- (void) peripheral:(CBPeripheral *)aPeripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
              error:(NSError *)error {
    if ([characteristic.UUID isEqual:m_receive_uuid]) {
        if(m_chunkData == CHUNK_DISABLED) {
            SEL didReceive = @selector(didReceive:);
            if ([delegate respondsToSelector:didReceive]) {
                [delegate didReceive:characteristic.value];
            }
        } else {
            [self processChunkData:characteristic.value];
        }
    }

}

- (bool) sendDataChunk: (NSData *)data withName:(NSString *)name {
    if(m_chunkData)
        free(m_chunkData);
    
    if([name length] > 18) {
        return false;
    }
    m_chunkData = malloc(sizeof(uint8_t)*([data length]+2+4+1));
    m_chunkDataName = name;
    
    // mark the start of the wav data block
    short startid = 0x06;
    memcpy(m_chunkData, &startid, 2);
    
    // add the size of the block
    m_chunkDataSize = (int)[data length];
    memcpy(&m_chunkData[2], &m_chunkDataSize, 4);
    
    // copy the data into the buffer
    [data getBytes:&m_chunkData[2+4] length:m_chunkDataSize];
    
    // add a checksum to the end of the data block
    uint8_t checksum = 0;
    int l = m_chunkDataSize;
    uint8_t *buffer = &m_chunkData[2+4];
    while(l--) checksum += *buffer++;
    m_chunkData[m_chunkDataSize+2+4] = ~checksum + 1;
    
    m_chunkDataSize += 2+4+1;
    m_curChunkIndex = 0;
    
    // we use a simple state machine to send the chunk data,
    // first up is to ask the peripheral if it is ready to receive the data
    NSData *rdata = [NSData dataWithBytes:(void*)&REQ_SENDCHUNKDATA length:2];
    [self sendData:rdata];
    m_chunkState = CHUNK_SENDREQ;

    [delegate updateLabel:@"Req data chunk send"];

    return TRUE;
}

//////////////////
//
// Send a chunk of data to the peripheral
//
- (void) processChunkData:(NSData *)data {
    uint8_t *p = (uint8_t*)[data bytes];
    NSUInteger len = [data length];
    
    if(m_chunkState == CHUNK_SENDREQ && len == 2) {
        short id = *(short *)p;
        if(id == ACK_SENDCHUNKDATA) {
            NSLog(@"Received ack from peripheral that it wants to receive chunk data");
            
            // peripheral ack that it can receive the chunk, send it the name
            // of the chunk data as step two
            uint8_t buf[20];
            short titleid = 0x04;
            memcpy(buf, &titleid, 2);
            strcpy((char *)&buf[2], [m_chunkDataName UTF8String]);
            NSData *data = [NSData dataWithBytes:(void*)buf length:2+[m_chunkDataName length]];
            [self sendData:data];
            m_chunkState = CHUNK_WAITINGRESP;
            [delegate updateLabel:@"Sending data chunk name"];
        }
    } else if(m_chunkState == CHUNK_WAITINGRESP) {
        short id = *(short *)p;
        if(id == ACK_READYTORECVCHUNKDATA) {
            NSLog(@"Received ack from peripheral that it will now receive the named data");
            
            char buf[20];
            memcpy(buf, &p[2], len-2);
            buf[len-2] = 0;

            NSString *dataName = [NSString stringWithUTF8String:buf];
            
            if([dataName isEqualToString:m_chunkDataName]) {
                // send the wav data payload (w/ simple checksum)
                m_chunkState = CHUNK_SENDCHUNKS;
                
                m_chunkTimer = [NSTimer scheduledTimerWithTimeInterval:(float)0.007 target:self
                                                             selector:@selector(sendChunkData) userInfo:nil repeats:NO];
                [delegate updateLabel:@"Sending data chunk"];
            } else {
                [delegate updateLabel:@"Error: data chunk name mismatch"];
            }
        }
    } else if(m_chunkState == CHUNK_FINISHEDCHUNKS && len == 2) {
        short id = *(short *)p;
        if(id == ACK_GOODCHUNKDATA) {
            NSLog(@"transmission of chunk data was a success!");
            [delegate updateLabel:@"Succesful trans data chunk"];
        } else if(id == ACK_BADCHUNKDATA) {
            NSLog(@"transmission of chunk data failed!");
            [delegate updateLabel:@"Error: Failed trans data chunk"];
        }
        
        m_chunkState = CHUNK_DISABLED;
    }
}

- (void) sendChunkData {
    if(m_chunkState == CHUNK_SENDCHUNKS) {
        if(m_curChunkIndex < m_chunkDataSize) {
            int len = 20;
            if(len > m_chunkDataSize - m_curChunkIndex) {
                len = m_chunkDataSize - m_curChunkIndex;
                m_chunkState = CHUNK_FINISHEDCHUNKS;
            }
            NSData *data = [NSData dataWithBytes:(void*)&m_chunkData[m_curChunkIndex] length:len];
        
            [self sendData:data];
            m_curChunkIndex += 20;
        
            m_chunkTimer = [NSTimer scheduledTimerWithTimeInterval:(float)0.007 target:self
                                                     selector:@selector(sendChunkData) userInfo:nil repeats:NO];
        } else if(m_chunkData) {
            free(m_chunkData);
            m_chunkData = NULL;
        }
    }
}

- (void) sendData: (NSData *)data {
    [m_peripheral writeValue:data forCharacteristic:m_send_characteristic type:CBCharacteristicWriteWithoutResponse];
}

-(void) disconnectPeripherals {
    [m_manager cancelPeripheralConnection: m_peripheral];
}

-(short) connectionState {
    return m_connectionState;
}


-(void)dealloc {
    NSLog(@"Cleaning up BluetoothLE");
}

@end
