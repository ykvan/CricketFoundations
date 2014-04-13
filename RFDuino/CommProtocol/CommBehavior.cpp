//////////////////////
//
// Communications behavior
//
#include "CommBehavior.h"
#include "BTLEMsgs.h"
#include "Arduino.h"
#include "SD.h"
#include "WavPlayback.h"

#define SD_ChipSelectPin 6

CommBehavior::CommBehavior(EventDispatcher *dispatcher, BTLEMsgs *btle) :
  FSM(dispatcher) {
  m_stateA = new ReadyState(this);
  m_stateB = new WavState(this, btle);

  m_stateA->AddTrigger(ON_CONNECT, m_stateA);
  m_stateA->AddTrigger(ON_DISCONNECT, m_stateA);
  m_stateA->AddTrigger(ON_PLAYWAV, m_stateA);
  m_stateA->AddTrigger(REQ_SENDWAVDATA, m_stateB);
  
  m_stateB->AddTrigger(WAVTITLEDATA, m_stateB);
  m_stateB->AddTrigger(ON_DISCONNECT, m_stateA);
  m_stateB->AddTrigger(ON_FINISHED_WAV_DOWNLOAD, m_stateA);

  m_currentState = m_stateA;

  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    Serial.println("SD fail");
  }
}

CommBehavior::~CommBehavior() {
  delete m_stateA;
  delete m_stateB;
}


ReadyState::ReadyState(FSM *fsm) :
  State(fsm),
  m_connected(false) {
}

ReadyState::~ReadyState() {
}

bool ReadyState::ProcessBTLEData(char *data, int len) {
  if (len == 2) {
    short id = *(short *)data;

    if (id == REQ_SENDWAVDATA) {
      m_fsm->GetDispatch()->SubmitEvent(REQ_SENDWAVDATA);
      return true;
    } else if(id == ON_PLAYWAV) {
      m_fsm->GetDispatch()->SubmitEvent(ON_PLAYWAV);
      return true;    
    }
  }
  return false;
}

void ReadyState::OnEntry(short eventid) {
  if (eventid == ON_CONNECT) {
    m_connected = true;
    Serial.println("ReadyState connected...\n");
  } else if (eventid == ON_DISCONNECT) {
    m_connected = false;
    Serial.println("ReadyState disconnected...\n");
  } else if (eventid == ON_PLAYWAV) {
     StartPlayback("test.wav");
  } else if (eventid == ON_FINISHED_WAV_DOWNLOAD) {
    Serial.println("Back from wav download...\n");
  }
}

WavState::WavState(FSM *fsm, BTLEMsgs *btle) :
  State(fsm),
  m_btle(btle),
  m_filename(NULL),
  m_wavdata(false) {

}

WavState::~WavState() {

}

void WavState::OnEntry(short eventid) {
  if (eventid == REQ_SENDWAVDATA) {
    Serial.println("Received request to send data");
    m_btle->SendData((char *)&ACK_SENDWAVDATA, 2);
    m_wavdata = false;
  }
}

void WavState::OnExit() {
  if(m_filename)
    delete []m_filename;
    
   if(m_file)
     m_file.close();
}

bool WavState::ProcessBTLEData(char *data, int len) {
  if (m_wavdata) {
    int l = len;
    uint8_t *buffer = (uint8_t *)data;
    while (l--) m_checksum += *buffer++;

    m_counter += len;

    if (m_counter >= m_dataSize) {
      int end = millis();
      float secs = (end - m_start) / 1000.0;
      float bps = m_dataSize / secs;
      Serial.print("bps: "); Serial.print(bps); Serial.print("\n");

      uint8_t test = ~m_checksum + 1;
      m_wavdata = false;

      if (test == 0) {
        m_file.write((const uint8_t*)data, len-1);
        
        m_btle->SendData((char *)&REC_WAVDATA_PASS, 2);
        Serial.println("rcv wav data success");
      } else {
        m_btle->SendData((char *)&REC_WAVDATA_FAIL, 2);
        Serial.println("rcv wav data failed");
      }
      m_fsm->GetDispatch()->SubmitEvent(ON_FINISHED_WAV_DOWNLOAD);
    } else {
      m_file.write((const uint8_t*)data, len);
    }
  } else if (len > 2) {
    short id = *(short *)data;

    if (id == WAVTITLEDATA) {
      m_filename = new char[len - 2 + 1];
      memcpy(m_filename, &data[2], len - 2);
      m_filename[len - 2] = 0;

      // delete the file on local storage...
      if (SD.exists(m_filename))
        SD.remove(m_filename);

      // send the ack msg (with the title)
      char buf[20];
      memcpy(buf, &REC_WAVTITLEDATA, 2);
      strcpy(&buf[2], m_filename);
      m_btle->SendData(buf, 2 + strlen(m_filename));
      Serial.print("Sending title name ack: "); Serial.print(2 + strlen(m_filename)); Serial.print(": "); Serial.println(m_filename);
      
      return true;
    } else if (id == WAVDATASTART) {
      m_wavdata = true;

      memcpy(&m_dataSize, &data[2], 4);
      m_counter = len - 6;

      Serial.print("Starting wav download: "); Serial.print(m_dataSize); Serial.print("\n");
      m_file = SD.open(m_filename, FILE_WRITE);
      m_file.write((const uint8_t*)&data[6], len-6);
      
      m_checksum = 0;
      int l = len - 6;
      uint8_t *buffer = (uint8_t *)&data[6];
      while (l--) m_checksum += *buffer++;

      m_start = millis();
    }
  }

  return false;
}
