//////////////////////
//
// Communications behavior
//
#pragma once
#include "StateMachine.h"
#include "Arduino.h"
#include "SD.h"

class BTLEMsgs;

class ReadyState : public State {
  public:
    ReadyState(FSM *fsm);
    virtual ~ReadyState();

    virtual bool ProcessBTLEData(char *data, int len);

  protected:
    virtual void OnEntry(short eventid);

    bool m_connected;
};

class WavState : public State {
  public:
    WavState(FSM *fsm, BTLEMsgs *btle);
    virtual ~WavState();

    virtual bool ProcessBTLEData(char *data, int len);
    
  protected:
    virtual void OnEntry(short eventid);
    virtual void OnExit();
    
    BTLEMsgs *m_btle;
    char *m_filename;
    bool m_wavdata;
    int m_dataSize;
    int m_counter;
    uint8_t m_checksum;
    
    File m_file;
    int m_start;
};

class CommBehavior : public FSM {
  public:
    CommBehavior(EventDispatcher *dispatch, BTLEMsgs *btle);
    virtual ~CommBehavior();
  protected:

    ReadyState *m_stateA;
    WavState *m_stateB;
};
