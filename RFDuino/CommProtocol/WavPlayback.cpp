#include "WavPlayback.h"
#include <SD.h>
#include <SPI.h>
#include <RFduinoBLE.h>


File sFile;

int sample = 0;
const byte buffSize = 1000; //note: there are 2 sound buffers. This will require (soundBuff*2) memory free
#define NO_BUFFER 2
volatile byte buffer[2][buffSize], curBuffer = NO_BUFFER, nextBuffer = 0, lastBuffer = NO_BUFFER;

int MAX_SAMPLE_LEVELS = 256UL;
unsigned int SAMPLE_RATE;

int PWM_OUTPUT_PIN_NUMBER = 2;        // hook up the speaker to this pin

static uint32_t last_cc0_sample;      /*!< CC0 register value in the previous round */
static uint32_t last_cc2_sample;      /*!< CC2 register value in the previous round */

uint32_t sampleVal = 0;     

bool playing = false;
bool cc0_turn = false; /*!< Variable to keep track which CC register is to be used */

static void gpiote_init(void) {

  
  turn_Off_GPIOTE_PPI_from_GPIO(PWM_OUTPUT_PIN_NUMBER);
  
  // Configure GPIOTE channel 0 to toggle the PWM pin state
  // Note that we can only connect one GPIOTE task to an output pin
  nrf_gpiote_task_config(0, PWM_OUTPUT_PIN_NUMBER, NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_LOW);
}

/** Initialises Programmable Peripheral Interconnect peripheral.
 */
static void ppi_init(void) {
  // Configure PPI channel 0 to toggle PWM_OUTPUT_PIN on every TIMER2 COMPARE[0] match  
  NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[0];
  NRF_PPI->CH[0].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];

  // Configure PPI channel 1 to toggle PWM_OUTPUT_PIN on every TIMER2 COMPARE[1] match
  NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[1];
  NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];

  // Configure PPI channel 1 to toggle PWM_OUTPUT_PIN on every TIMER2 COMPARE[2] match
  NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER1->EVENTS_COMPARE[2];
  NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];

  // Enable PPI channels 0-2
  NRF_PPI->CHEN = (PPI_CHEN_CH0_Enabled << PPI_CHEN_CH0_Pos)
                | (PPI_CHEN_CH1_Enabled << PPI_CHEN_CH1_Pos)
                | (PPI_CHEN_CH2_Enabled << PPI_CHEN_CH2_Pos);
}

static void timer1_init(void) {

  NRF_TIMER1->MODE = TIMER_MODE_MODE_Timer;
  NRF_TIMER1->PRESCALER = 0;

  // Clears the timer, sets it to 0
  NRF_TIMER1->TASKS_CLEAR = 1;

  // Load initial values to TIMER2 CC registers.
  // CC2 will be set on the first CC1 interrupt.
  // Timer compare events will only happen after the first 2 values
  last_cc0_sample = sampleVal;
  last_cc2_sample = 0;
  NRF_TIMER1->CC[0] = MAX_SAMPLE_LEVELS + last_cc0_sample;
  NRF_TIMER1->CC[1] = MAX_SAMPLE_LEVELS;
  NRF_TIMER1->CC[2] = 0;

  // Interrupt setup
  NRF_TIMER1->INTENSET = (TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos);
  
  attachInterrupt(TIMER1_IRQn, TIMER1_IRQHandler);    // also used in variant.cpp to configure the RTC1
  cc0_turn = false;
  NRF_TIMER1->TASKS_START = 1;
}

void TIMER1_IRQHandler(void) {

  if ((NRF_TIMER1->EVENTS_COMPARE[1] != 0) && ((NRF_TIMER1->INTENSET & TIMER_INTENSET_COMPARE1_Msk) != 0))
  {
    // Sets the next CC1 value
    NRF_TIMER1->EVENTS_COMPARE[1] = 0;
    NRF_TIMER1->CC[1] = (NRF_TIMER1->CC[1] + MAX_SAMPLE_LEVELS);

    // Every other interrupt CC0 and CC2 will be set to their next values
    // They each keep track of their last duty cycle so they can compute their next correctly
    uint32_t next_sample = sampleVal;

    if (cc0_turn)
    {
      NRF_TIMER1->CC[0] = (NRF_TIMER1->CC[0] - last_cc0_sample + 2*MAX_SAMPLE_LEVELS + next_sample);
      last_cc0_sample = next_sample;
    }
    else
    {
      NRF_TIMER1->CC[2] = (NRF_TIMER1->CC[2] - last_cc2_sample + 2*MAX_SAMPLE_LEVELS + next_sample);
      last_cc2_sample = next_sample;
    }
    // Next turn the other CC will get its value
    cc0_turn = !cc0_turn;
  }
}

void TIMER2_IRQHandler(void) {
  // simple tone generator - replace this with code to step through
  // PCM data at the required sample rate...
  NRF_TIMER2->EVENTS_COMPARE[0] = 0;
  
  if(curBuffer != NO_BUFFER) {
    sampleVal = buffer[curBuffer][sample];
    ++sample;
    if(sample >= buffSize) {
      if(curBuffer == lastBuffer)
        StopPlayback();
      else {
        nextBuffer = curBuffer;
        curBuffer = !curBuffer;
        sample = 0;
      }
    }
    if(sampleVal == 0)
      sampleVal = 1; 
  }
}

static void timer2_init(void) {
  NRF_TIMER2->TASKS_STOP = 1;   
  NRF_TIMER2->MODE = TIMER_MODE_MODE_Timer; 
  NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
  NRF_TIMER2->PRESCALER = 4;  
  NRF_TIMER2->TASKS_CLEAR = 1; 
  NRF_TIMER2->CC[0] = (1000000/SAMPLE_RATE);
  NRF_TIMER2->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;  
  NRF_TIMER2->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
  attachInterrupt(TIMER2_IRQn, TIMER2_IRQHandler);   
  NRF_TIMER2->TASKS_START = 1;
}

boolean wavInfo(char* filename) {
  //check for the string WAVE starting at 8th bit in header of file
  File xFile = SD.open(filename, FILE_READ);
  if(!xFile){return 0;}
  xFile.seek(8);
  char wavStr[] = {'W','A','V','E'};
  for (int i =0; i<4; i++){
	  if(xFile.read() != wavStr[i]){ Serial.println("WAV File Error"); break; }
  }

  xFile.seek(22);
  int numChannels = xFile.read();
  numChannels = xFile.read() << 8 | numChannels;
  Serial.print("Num channels = ");
  Serial.println(numChannels);
  
  xFile.seek(24);
  SAMPLE_RATE = xFile.read();
  SAMPLE_RATE = xFile.read() << 8 | SAMPLE_RATE;

  Serial.println(SAMPLE_RATE);
  
  //verify that Bits Per Sample is 8 (0-255)
  xFile.seek(34); unsigned int dVar = xFile.read();
  dVar = xFile.read() << 8 | dVar;
  if(dVar != 8){Serial.print("Wrong BitRate"); xFile.close(); return 0;}
  xFile.close(); return 1;
}

void StartPlayback(char* filename) {
  if(playing)
    StopPlayback();
  
  if(!wavInfo(filename))
    return; 
  
  Serial.println("Opening wav file");
  
  sFile = SD.open(filename, FILE_READ);
  if(sFile) {
    sFile.seek(44); //skip the header info
    
    sFile.read((byte*)&buffer[0],buffSize);
    sample = 0;
    curBuffer = 0;
    nextBuffer = 1;
    lastBuffer = NO_BUFFER;
    RFduinoBLE.end();
    
    timer2_init(); 
    timer1_init();
    gpiote_init();
    ppi_init();

    Serial.println("start playback");
    playing = true;
  }
}
void StopPlayback() {
  NRF_TIMER1->TASKS_STOP = 1;
  NRF_TIMER2->TASKS_STOP = 1;
 
  if(sFile) 
    sFile.close();
  
  playing = false;
  Serial.println("stop playback");
  RFduinoBLE.begin();
}

void UpdateWavPlayback() {
  if(playing && nextBuffer != NO_BUFFER) {
    int size = sFile.read((byte*)&buffer[nextBuffer],buffSize);
    if(size < buffSize) {
      StopPlayback();
    }
    nextBuffer = NO_BUFFER;
  } 
}
