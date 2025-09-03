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
uint32_t io_keys_map[NUM_TOTAL_KEYS] = {
    // board 0
    0b000000000000100000, //p0
    0b000000000001000000, //p1
    0b000000000010000000, //p2
    0b000000000100000000, //p3
    0b000000001000000000, //p4
    0b000000010000000000, //p5
    0b000000000000010000, //p6
    0b000000000000001000, //p7
    0b000000000000000100, //p8
    0b000000000000000010, //p9
    0b000000000000000001, //p10
    // board 1
    0b000000100000000000, //p0
    0b000001000000000000, //p1
    0b000010000000000000, //p2
    0b000100000000000000, //p3
    0b001000000000000000, //p4
    0b010000000000000000, //p5
    0b100000000000000000  //p6
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

int32_t keys_octaves_map[NUM_OCTAVE_KEYS][2] = {
    {0b00000100, 0},
    {0b00000000, 0},
    {0b00000000, 0},
    {0b00000000, 0},
    {0b00000000, 0},
};

// define sensor -> key map
// define a number in binary with one bit=1 for each button. if that button is held, it is added to the new KEYS_HELD value. that way we just do like 17 sums
Adafruit_MPR121 b0 = Adafruit_MPR121();
Adafruit_MPR121 b1 = Adafruit_MPR121();

uint8_t B_DATAPIN = 8;
uint8_t B_CLOCKPIN = 9;
HX710B BreathSensor(B_DATAPIN, B_CLOCKPIN);

// loop-local definitions
uint32_t m_last_keys_held = 0b0000000000000000;
int16_t m_last_valid_note = 0;

uint32_t m_allbuttons_held = 0;
uint32_t m_allkeys_held = 0b0000000000000000;
uint32_t m_octaves = 0;
uint32_t m_buttons = 0;

uint16_t touched_b0 = 0;
uint16_t touched_b1 = 0;
int32_t breath_value = 0;
int16_t note_value = 0;

void setup()
{
  // set pins as input/output
  // start any communication steps
  Wire.begin();
  b0.begin(0x5A, &Wire);
  b1.begin(0x5B, &Wire);

  b0.setAutoconfig(true);
  b1.setAutoconfig(true);

  BreathSensor.begin();
  BreathSensor.calibrate(50, 0, 100, 80);
}

void loop()
{
  // read from inputs
  touched_b0 = b0.touched();
  touched_b1 = b1.touched();
  breath_value = BreathSensor.read();

  // turn into logical values
  m_allbuttons_held = ki(touched_b0, NUM_KEYS_B0) + (ki(touched_b1, NUM_KEYS_B1) << NUM_KEYS_B0);
  
  m_allkeys_held = 0;
  // map the IO to the keys in order
  for (int i=0; i<NUM_TOTAL_KEYS; i++) {
    m_allkeys_held += 0; // future me: figure out a magical way to do this using the button map
  }
  
  // filter octave out, get keys only
  m_octaves = ki(m_allkeys_held, NUM_OCTAVE_KEYS);
  m_allkeys_held = ki(m_allkeys_held >> NUM_OCTAVE_KEYS, NUM_NOTE_KEYS);

  note_value = getNote(m_allkeys_held);
}

int ki(int input_touched, int number_interest)
{
  return (input_touched & ((1 << number_interest) - 1));
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