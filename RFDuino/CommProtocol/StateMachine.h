//////////////////////
//
// State machine
//
#pragma once
#include <stdlib.h>

class State;
class FSM;
class EventDispatcher;

class Trigger {
  public:
    Trigger(short eventid, State *dest) : m_eventId(eventid), m_dest(dest), m_next(NULL) {}
    ~Trigger() {}

    short m_eventId;
    State *m_dest;
    Trigger *m_next;
};

class State {
    friend class FSM;
  public:
    State(FSM *fsm);
    virtual ~State();

    // add triggers out of this state
    bool AddTrigger(short eventType, State *dest);

    void OnTrigger(short eventid);

    // outside the update loop, handle the special case
    // of BTLE data packet processing
    virtual bool ProcessBTLEData(char *data, int len) {
      return false;
    }

  protected:
    virtual void OnEntry(short eventid)  {}
    virtual void OnExit()   {}

    FSM *m_fsm;
    Trigger *m_triggers;
};

class EventQueue {
  public:
    EventQueue(short eventId) : m_eventId(eventId), m_next(NULL) {}

    short m_eventId;
    EventQueue *m_next;
};

//////////////////////
//
// Finite state machine driving the behavior of the device
//
class FSM {
  friend class EventDispatcher;
  public:
    FSM(EventDispatcher *dispatch) : m_currentState(NULL), m_dispatch(dispatch), m_next(NULL) {}
    virtual ~FSM() {}

    void SetCurrentState(State *state, const short event);

    // we have a standard event/trigger state machine but have also
    // added a special case handler for BTLE data packets to get
    // processed as they come in outside the update loop
    void SubmitBTLEData(char *data, int len);

    void ProcessEvent(short eventid);

    EventDispatcher *GetDispatch()  {
      return m_dispatch;
    }

  protected:
    EventDispatcher *m_dispatch;
    State *m_currentState;
    FSM *m_next;
};

//////////////////////
//
// Container of behaviors. Responsible for dispatching
// events to the behavors.
//
class EventDispatcher {
  public:
    EventDispatcher();
    ~EventDispatcher();

    void AddFSM(FSM *fsm);

    void SubmitBTLEData(char *data, int len);

    void SubmitEvent(const short id);
    void ProcessEvents(int tick);

    int GetTick()  {
      return m_currentTick;
    }

  protected:
    FSM *m_fsms;
    EventQueue *m_events;
    int m_currentTick;
};

