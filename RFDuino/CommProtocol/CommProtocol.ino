///////////////////
//
// Communication protocol between the rfduino and the central.
// BTLE packets are 20 bytes. Each message will start with at 
// least a 2 byte msg identifier header. For messages with additional payload
// (based on the identifier) an additional 4 bytes will make up the header
// giving the size of the payload. Additionally, the last 4 bytes of the payload
// will be a "special" EOM tag.
//
#include "BTLEMsgs.h"
#include <RFduinoBLE.h>
#include "SD.h"
#include "SPI.h"
#include "StateMachine.h"
#include "CommBehavior.h"
#include "WavPlayback.h"

EventDispatcher dispatch;
BTLEMsgs comm(&dispatch);

void setup() {
  comm.Setup();  
  
  dispatch.AddFSM(new CommBehavior(&dispatch, &comm));  
}

void loop() {
   dispatch.ProcessEvents(millis());
   
   UpdateWavPlayback();
}
