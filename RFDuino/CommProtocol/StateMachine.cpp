//////////////////////
//
// State
//
#include "StateMachine.h"
#include "Arduino.h"

void FSM::SetCurrentState(State *state, const short trigger) {
  if (m_currentState)
    m_currentState->OnExit();

  m_currentState = state;
  m_currentState->OnEntry(trigger);
}

void FSM::ProcessEvent(short eventid) {
  if (m_currentState)
    m_currentState->OnTrigger(eventid);
}

void FSM::SubmitBTLEData(char *data, int len) {
  if (m_currentState)
    m_currentState->ProcessBTLEData(data, len);
}

State::State(FSM *fsm) :
  m_fsm(fsm) {

}

State::~State() {
  Trigger *t = m_triggers;
  while (t) {
    Trigger *n = t->m_next;
    delete t;
    t = n;
  }
}

bool State::AddTrigger(const short eventType, State *dest) {
  Trigger *t = new Trigger(eventType, dest);
  t->m_next = m_triggers;
  m_triggers = t;

  return true;
}

void State::OnTrigger(short eventid) {
  Trigger *t = m_triggers;
  while (t) {
    Trigger *n = t->m_next;
    if (t->m_eventId == eventid) {
      m_fsm->SetCurrentState(t->m_dest, eventid);
      break;
    }
    t = n;
  }
}


EventDispatcher::EventDispatcher() :
  m_currentTick(0) {

}

EventDispatcher::~EventDispatcher() {
  FSM *f = m_fsms;
  while (f) {
    FSM *n = f->m_next;
    delete f;
    f = n;
  }
}

void EventDispatcher::AddFSM(FSM *fsm) {
  fsm->m_next = m_fsms;
  m_fsms = fsm;
}

void EventDispatcher::SubmitBTLEData(char *data, int len) {
  FSM *f = m_fsms;
  while (f) {
    FSM *n = f->m_next;
    f->SubmitBTLEData(data, len);
    f = n;
  }
}

void EventDispatcher::SubmitEvent(const short id) {
  EventQueue *e = new EventQueue(id);
  e->m_next = m_events;
  m_events = e;
}

void EventDispatcher::ProcessEvents(int tick) {
  m_currentTick = tick;

  EventQueue *e = m_events;
  m_events = NULL;

  while (e) {
    EventQueue *n = e->m_next;
    FSM *f = m_fsms;
    while (f) {
      FSM *n = f->m_next;
      f->ProcessEvent(e->m_eventId);
      f = n;
    }
    delete e;
    e = n;
  }
}
