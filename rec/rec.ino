
/*
 * =======
 *   REC
 * =======
 * 
 * REC is a record-and-play Eurorack module
 * 
 * Copyright 2024 David Haillant
 * www.davidhaillant.com
 * 
 * 
 * Compatible hardware:
 * REC V0.1 (20240315)
 * 
 */

/* 
 * This program is free software: you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <https://www.gnu.org/licenses/>. 
 */



/* 
 * 
 * How does it work?
 * =================
 * 
 * In Record mode, the voltage present at "CV In" pin is sampled and stored when Clock input receive a rising edge
 * Up to 512 samples can be stored
 * 
 * In Play mode, the PWM output will playback the stored samples, in the same sequence as recorded, on each pulse on Clock input
 * 
 * If Reset input receives a pulse, the index of stored values goes back to the begining, both in Record and Play mode.
 * In Play mode, the playback starts from the begining.
 * In Record mode, the previous samples are overwritten.
 * 
 * The last sample is the last sample stored in Record mode.
 * In play mode, if the last sample has been reached, the next sample to be played will be the first in the memory.
 * 
 * To append new recordings, switch to Record mode and send pulses on Clock
 * 
 * While recording, the CV output will reproduce immediately the recorded sample
 * 
 */


#define DEBUG


// easy way to set PWM outputs
#include <PWMFreak.h>


// Inputs and Outputs *************************************
// inputs
#define CLOCK_IN_PIN     2
#define RESET_IN_PIN     3
#define PLAY_REC_IN_PIN  4

// ADC (AVR name)
#define CV_IN_PIN     PC5

// LEDs
#define PLAY_REC_LED_PIN  5
#define CLOCK_LED_PIN     6
#define RESET_LED_PIN     7

// PWM pins
#define PITCH_MSB_PIN   9   // D9/D10 (output 5)
#define PITCH_LSB_PIN   10  // D9/D10 (output 5)


#define KEY_NONE      0
#define KEY_PLAY_REC  1
#define KEY_STEP      2
#define KEY_RESET     3

#define KEYDETECTOR
#ifdef KEYDETECTOR
  #include <KeyDetector.h>

  Key keys[] = {{KEY_PLAY_REC, PLAY_REC_IN_PIN}, {KEY_STEP, CLOCK_IN_PIN}, {KEY_RESET, RESET_IN_PIN}};

  KeyDetector myKeyDetector(keys, sizeof(keys)/sizeof(Key), 10, 0, true);
  // true is for PULLUP
#endif


#define MAX_SAMPLES 512         // 1024 bytes (16 bits x 512)
uint16_t samples[MAX_SAMPLES];

uint16_t last_sample = 0;   // up to 65536
uint16_t rw_position = 0;

enum Mode {Record, Play} mode;


void setup()
{
  #ifdef DEBUG
    Serial.begin(115200);
    Serial.println(F("REC!"));
  #endif

  // set Inputs
  pinMode(CLOCK_IN_PIN,    INPUT_PULLUP);
  pinMode(RESET_IN_PIN,    INPUT_PULLUP);
  pinMode(PLAY_REC_IN_PIN, INPUT_PULLUP);

  // set LED Outputs
  pinMode (PLAY_REC_LED_PIN, OUTPUT);
  pinMode (CLOCK_LED_PIN,    OUTPUT);
  pinMode (RESET_LED_PIN,    OUTPUT);

  // PWM pins
  pinMode (PITCH_MSB_PIN, OUTPUT);
  pinMode (PITCH_LSB_PIN, OUTPUT);

  // change PWM for use divisor value 1
  setPwmFrequency(9, 1);  // Pins 9 and 10 are paired on timer1


  last_sample = 0;
  rw_position = 0;

  samples[0] = 0;

  mode = Play;

  // hello world!
  for (uint8_t i = 0; i < 4; i++)
  {
    digitalWrite(PLAY_REC_LED_PIN, HIGH);
    delay(200);
    digitalWrite(PLAY_REC_LED_PIN, LOW);
    delay(0);
    digitalWrite(CLOCK_LED_PIN, HIGH);
    delay(100);
    digitalWrite(CLOCK_LED_PIN, LOW);
    delay(0);
    digitalWrite(RESET_LED_PIN, HIGH);
    delay(100);
    digitalWrite(RESET_LED_PIN, LOW);
    delay(0);
  }

  #ifdef DEBUG
    Serial.print(F("MAX_SAMPLES: "));
    Serial.println(MAX_SAMPLES);
  #endif

  setup_adc();

  update_LEDs();
}

void setup_adc(void)
{
  cli(); // Disable interrupts.

  // Set up continuous sampling of analog pin 0.

  // Clear ADCSRA and ADCSRB registers.
  ADCSRA = 0;
  ADCSRB = 0;

  ADMUX |= (1 << REFS0);  // Set reference voltage.
  //ADMUX |= (1 << ADLAR);  // Left align the ADC value - so we can read highest 8 bits from ADCH register only
  ADMUX |= PC5;           // Read on pin PC5

  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); // Set ADC clock with 32 prescaler -> 16mHz / 32 = 500kHz.
  ADCSRA |= (1 << ADATE); // Enable auto trigger.
  //ADCSRA |= (1 << ADIE);  // Enable interrupts when measurement complete.
  ADCSRA |= (1 << ADEN);  // Enable ADC.
  ADCSRA |= (1 << ADSC);  // Start ADC measurements.

  sei(); // Enable interrupts.
}

void render_cv(void)
{
  analogWrite(PITCH_MSB_PIN, (samples[rw_position] & 0xff00) >> 8);
  analogWrite(PITCH_LSB_PIN, samples[rw_position] & 0xff);

  #ifdef DEBUG
    Serial.print(F("Sample ["));
    Serial.print(rw_position);
    Serial.print(F("] = "));
    Serial.println(samples[rw_position]);
    Serial.print(F("last: "));
    Serial.println(last_sample);
  #endif
}

void update_LEDs(void)
{
  digitalWrite(PLAY_REC_LED_PIN, mode);
}

void switch_mode(void)
{
  mode = (mode == Play) ? Record : Play;                  // change Mode
  rw_position = (mode == Play) ? 0 : rw_position;   // reset index only if switching from Record to Play

  #ifdef DEBUG
    Serial.print(F("MODE: "));
    Serial.print(mode);
    if (mode == 0) {Serial.println(F(" record"));} else {Serial.println(F(" play"));}
  #endif

  update_LEDs();
}

void record_new_sample(void)
{
  if (rw_position < MAX_SAMPLES)
  {
    samples[rw_position] = ADC;

    #ifdef DEBUG
      Serial.print(F("REC: "));
    #endif

    last_sample = rw_position;   // remember the number of recorded samples
    render_cv();
    rw_position++;               // ready for next sample
  }
  else
  {
    // show some error
    // do not record anymore
  }
}

void play_sample(void)
{
  #ifdef DEBUG
    Serial.print(F("PLAY: "));
  #endif

  rw_position = (rw_position > last_sample) ? 0 : rw_position;   // reset index if end has been reached
  render_cv();
  rw_position++;
}

void reset_position(void)
{
  rw_position = 0;
  if (mode == Record)
  {
    last_sample = 0;
  }
  else  // Play mode
  {
    // continue
  }
  #ifdef DEBUG
    Serial.print(F("RESET: "));
  #endif
  render_cv();
}

void loop()
{
/*  
  if button mode pressed
    if previous mode was REC, then reset index to 0

  if rising edge on Clock detected,
    if in REC mode,
      if index < max samples
        store ADC reading in samples at index
        increment index
        last sample = current index
      else show error

    if in PLAY mode

      output sample at index
      increment index
      if index is > last sample then reset index to 0

  if RESET detected
    if in REC mode,
      reset index, reset last sample
    if in PLAY mode
      reset index
*/

  uint8_t trigger = false;

  myKeyDetector.detect();           // read keys and jack inputs
  trigger = myKeyDetector.trigger;  // store the name of the pressed key, if detected

  if (trigger)                // if a key has been pressed (or a rising edge on jack inputs)
  {
    switch (trigger)
    {
      case KEY_PLAY_REC:          // button "Play/Rec" has been pressed
        switch_mode();
        break;
      case KEY_STEP:              // button "Step" has been pressed or "Clock" jack input has received a rising edge
        if (mode == Record)
        {
          record_new_sample();
        }
        else  // Play mode
        {
          play_sample();
        }
        break;
      case KEY_RESET:             // button "Reset" has been pressed or "Reset" jack input has received a rising edge
        reset_position();
        break;
      case KEY_NONE:
        break;
    }
  }
}

/*
ISR(ADC_vect) {       // When new ADC value ready.
  #ifdef DEBUG
    //Serial.println(ADC);
  #endif
}
*/
