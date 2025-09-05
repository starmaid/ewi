// written by starmaid september 2025

#include <Adafruit_MPR121.h>
#include <Wire.h>

#include <MIDIUSB.h>
#include <MIDIUSB_Defs.h>
#include <pitchToFrequency.h>
#include <frequencyToNote.h>
#include <pitchToNote.h>

#include "HX710AB.h"

// Machine state and constants. state not currently used.
enum DeviceState
{
  Initializing, // 0
  Playing,      // 1
};

int DEVICE_MODE = DeviceState::Initializing;

const uint8_t NUM_IO = 32;
const uint8_t NUM_KEYS_B0 = 11;
const uint8_t NUM_KEYS_B1 = 7;
const uint8_t NUM_OCTAVE_KEYS = 5;
const uint8_t NUM_TOTAL_KEYS = NUM_KEYS_B0 + NUM_KEYS_B1;
const uint8_t NUM_NOTE_KEYS = NUM_TOTAL_KEYS - NUM_OCTAVE_KEYS;

// Thresholds - not used currently
int note_on_pressure_threshold = 0;
int button_pressed_cap_threshold = 0;

// define input to keys
// this should match the wiring you make.
uint32_t io_keys_map[NUM_IO] = {
    // board 0
    0b000000000000000000,
    0b000000000000000000, //p0 // the first two pins are broken?
    0b000000000000100000, //p1
    0b000000000001000000, //p2
    0b000000000010000000, //p3
    0b000000000100000000, //p4
    0b000000001000000000, //p5
    0b000000010000000000, //p6
    0b000000000000010000, //p7
    0b000000000000001000, //p8
    0b000000000000000100, //p9
    0b000000000000000010, //p10
    0b000000000000000001, //p11 unused
    0b000000000000000000, //x12 
    0b000000000000000000, //x13
    0b000000000000000000, //x14
    // board 1
    0b000000100000000000, //p0 
    0b000001000000000000, //p1
    0b000010000000000000, //p2
    0b000100000000000000, //p3
    0b001000000000000000, //p4
    0b010000000000000000, //p5
    0b100000000000000000, //p6
    0b000000000000000000, //p7
    0b000000000000000000, //p8
    0b000000000000000000, //p9
    0b000000000000000000, //p10
    0b000000000000000000, //p11
    0b000000000000000000, //x12
    0b000000000000000000, //x13
    0b000000000000000000, //x14
    0b000000000000000000  //x15
  };

// define EWI fingerings
// write in binary which buttons are pressed and associate with a note.
const uint8_t NUM_FINGERINGS = 18;
int32_t keys_notes_map[NUM_FINGERINGS][2] = {
    {0b1011010111011, pitchB2b},
    {0b1011010111001, pitchB2},
    {0b1011000111001, pitchC3},
    {0b1011100111001, pitchD3b},
    {0b1011000111000, pitchD3},
    {0b1011000111100, pitchE3b},
    {0b1011000110000, pitchE3},
    {0b1011000100000, pitchF3},
    {0b1011000010000, pitchG3b},
    {0b1011000000000, pitchG3},
    {0b1011100000000, pitchA3b},
    {0b1010000000000, pitchA3},
    {0b1010001000000, pitchB3b},
    {0b1100000000000, pitchB3b},
    {0b1000000000000, pitchB3},
    {0b0010000000000, pitchC4},
    {0b0000000000000, pitchD4b},
    {0b0000100000000, pitchD4}};

const uint8_t NUM_OCTAVE_FINGERINGS = 5;
int32_t keys_octaves_map[NUM_OCTAVE_FINGERINGS][2] = {
    {0b00000100, 0},
    {0b00000010, -12},
    {0b00000001, -24},
    {0b00001000, 12},
    {0b00010000, 24},
};

// define sensor -> key map
// define a number in binary with one bit=1 for each button. if that button is held, it is added to the new KEYS_HELD value. that way we just do like 17 sums
Adafruit_MPR121 b0 = Adafruit_MPR121();
Adafruit_MPR121 b1 = Adafruit_MPR121();

uint8_t B_DATAPIN = 8;
uint8_t B_CLOCKPIN = 9;
HX710B BreathSensor(B_DATAPIN, B_CLOCKPIN);
//HX711 BreathSensor;

// loop-local definitions
uint32_t m_last_keys_held = 0b0000000000000000;
byte m_last_valid_note = 0;
byte m_last_valid_breath = 0;

uint32_t m_allbuttons_held = 0;
uint32_t testbit = 0;
uint32_t m_allkeys_held = 0b0000000000000000;
uint32_t m_octaves = 0;
uint32_t m_buttons = 0;

uint16_t touched_b0 = 0;
uint16_t touched_b1 = 0;
int32_t breath_value = 0;

byte breath_cc = 0;
byte note_value = 0;
bool note_playing = false;
byte deadzone = 5;
byte velocity = 64;

byte easy_factor = 90;

byte midi_channel = 5;
byte midi_cc_channel = 0;

void setup()
{
  delay(1000);
  // set pins as input/output
  // start any communication steps
  Wire.begin();
  b0.begin(0x5A, &Wire);
  b1.begin(0x5B, &Wire);

  b0.setAutoconfig(true);
  b1.setAutoconfig(true);
  
  BreathSensor.begin();
  //BreathSensor.begin(B_DATAPIN, B_CLOCKPIN);
  //BreathSensor.calibrate(50, 0, 100, 80);
  
  Serial.begin(115200);
  Serial.println();
}

void loop()
{
  // read from inputs
  touched_b0 = b0.touched();
  touched_b1 = b1.touched();
  
  //if (BreathSensor.wait_ready_retry(1,0)) {
    breath_value = BreathSensor.read(true);
  //}

  // turn into logical values
  m_allbuttons_held = touched_b0 + ((uint32_t) touched_b1 << 16);

  #ifdef DEBUG
  Serial.print(m_allbuttons_held + ((uint32_t) 1<<31), BIN);
  Serial.print(" ");
  #endif
  
  m_allkeys_held = 0;
  // map the IO to the keys in order
  for (int i=0; i<NUM_IO; i++) {
    testbit = (uint32_t) 1 << i;
    if (m_allbuttons_held & testbit) {
      m_allkeys_held += io_keys_map[i];
    }
  }
  #ifdef DEBUG
  Serial.print(m_allkeys_held + ((uint32_t) 1<<31), BIN);
  Serial.print(" ");
  #endif
  
  
  
  breath_cc = map(breath_value, -8388610, 8388610, 0,255);
  Serial.print(breath_cc);
  
  // filter octave out, get keys only
  m_octaves = ki(m_allkeys_held, NUM_OCTAVE_KEYS);
  m_allkeys_held = m_allkeys_held >> NUM_OCTAVE_KEYS;

  note_value = getNote(m_allkeys_held);
  note_value = note_value + getOctave(m_octaves);
  
  #ifdef DEBUG
  Serial.print(" ");
  Serial.print(m_allkeys_held + ((uint32_t) 1<<16), BIN);
  Serial.print(" ");
  Serial.print(note_value, DEC);
  #endif
  Serial.println();
  
  if (!note_playing && (breath_cc > 127 + deadzone || breath_cc < 127 - deadzone)) {
    noteOn(midi_channel, note_value, velocity);
    note_playing = true;
  } else if (note_playing && note_value != m_last_valid_note) {
    noteOff(midi_channel,m_last_valid_note,velocity);
    noteOn(midi_channel, note_value, velocity);
  } else if (note_playing && ! (breath_cc > 127 + deadzone || breath_cc < 127 - deadzone)) {
    noteOff(midi_channel,m_last_valid_note,velocity);
    note_playing = false;
  }
  
  if (breath_cc != m_last_valid_breath){
    // positive pressure
    controlChange(midi_channel, midi_cc_channel, constrain(map(breath_cc, 127,255 - easy_factor,0,127),0,127));
    // negative pressure
    controlChange(midi_channel, midi_cc_channel+1, constrain(map(breath_cc, 127,0 + easy_factor,0,127),0,127));
  }
  

  
  MidiUSB.flush();
  
  m_last_valid_note = note_value;
  m_last_valid_breath = breath_cc;
  delay(1);
  return;
}


int16_t getNote(uint32_t keys_held)
{
  for (uint8_t i = 0; i < NUM_FINGERINGS; i++)
  {
    if (keys_held == (uint32_t) keys_notes_map[i][0])
    {
      return keys_notes_map[i][1];
    }
  }
  return m_last_valid_note;
}

int16_t getOctave(uint32_t keys_held)
{
  for (uint8_t i = 0; i < NUM_OCTAVE_FINGERINGS; i++)
  {
    if (keys_held == (uint32_t) keys_octaves_map[i][0])
    {
      return keys_octaves_map[i][1];
    }
  }
  return 0;
}

int ki(int input_touched, int number_interest)
{
  return (input_touched & ((1 << number_interest) - 1));
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}
