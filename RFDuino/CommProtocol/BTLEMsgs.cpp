///////////////////
//
// State machine for handling the rfduino/central communications system.
//
#include "BTLEMsgs.h"
#include "StateMachine.h"
#include <RFduinoBLE.h>
#include "Arduino.h"

BTLEMsgs *g_btleMsgs = 0;

BTLEMsgs::BTLEMsgs(EventDispatcher *dispatch) :
  m_dispatch(dispatch),
  m_connected(false) {
}

BTLEMsgs::~BTLEMsgs() {
  if (g_btleMsgs == this)
    g_btleMsgs = 0;
}

void BTLEMsgs::Setup() {
  Serial.begin(9600);
  Serial.println("Waiting for connection...");

  g_btleMsgs = this;
  RFduinoBLE.begin();
}

void BTLEMsgs::Connected(bool c) {
  m_connected = c;
  if (c) {
    m_dispatch->SubmitEvent(ON_CONNECT);
  } else {
    m_dispatch->SubmitEvent(ON_DISCONNECT);
  }
}
void BTLEMsgs::SendData(char *data, int len) {
  while (! RFduinoBLE.send(data, len))
      ;
}

void BTLEMsgs::OnReceivedData(char *data, int len) {
  m_dispatch->SubmitBTLEData(data, len);
}

void RFduinoBLE_onConnect() {
  Serial.println("Connected");
  if (g_btleMsgs)
    g_btleMsgs->Connected(true);
}

void RFduinoBLE_onDisconnect() {
  Serial.println("Disconnected");
  if (g_btleMsgs)
    g_btleMsgs->Connected(false);

}

void RFduinoBLE_onReceive(char *data, int len) {
  if(len <= 20) {
    g_btleMsgs->OnReceivedData(data, len);
  }
}

