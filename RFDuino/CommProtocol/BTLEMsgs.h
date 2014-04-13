///////////////////
//
// State machine for handling the rfduino/central communications system.
//
#pragma once

// state machine event ids
const short ON_CONNECT = 0x00;
const short ON_DISCONNECT = 0x01;
const short ON_FINISHED_WAV_DOWNLOAD = 0x09;          
const short ON_PLAYWAV = 0x0a;

// 
const short REQ_SENDWAVDATA = 0x02;       // central asking if it can send wav data to the rfduino
const short ACK_SENDWAVDATA = 0x03;       // rfduino ack that it is in a state where it can receive wav data
const short WAVTITLEDATA = 0x04;          // central sending the filename of the wav data
const short REC_WAVTITLEDATA = 0x05;      // rfduino ack receiving the wav filename (with the name)
const short WAVDATASTART = 0x06;          // central sending the first packet of wav data
const short REC_WAVDATA_PASS = 0x07;      // rfduino ack receiving the wav data (pass)
const short REC_WAVDATA_FAIL = 0x08;      // rfduino ack receiving the wav data (fail)

class EventDispatcher;

class BTLEMsgs {
  public:

    BTLEMsgs(EventDispatcher *dispatch);
    ~BTLEMsgs();

    void Setup();

    void Connected(bool c);

    void OnReceivedData(char *data, int len);

    void SendData(char *data, int len);

  protected:

    EventDispatcher *m_dispatch;
    bool m_connected;

};
