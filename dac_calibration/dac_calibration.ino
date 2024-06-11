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
 * ----------------
 *  DAC Calibation
 * ----------------
 * 
 * Connect a Voltmeter with good accuracy and precision (1mV or lower) in VDC mode to CV Out socket
 * Let the circuit stabilizes its temperature 10mn or so before doing adjustments
 * Before trying to adjust the 4 octaves (from 1V to 4V), null out the DC offset for the 0V output by setting the multi turn trimmer on the PCB.
 * 
 * Cycle through octaves 1, 2, 3 and 4 (1V, 2V, 3V and 4V) by pressing STEP button
 * and adjust precisely the output voltage by pressing REC button for increasing the output by few mV
 * or decrease output voltage by few mV by pressing RESET button
 * 
 * Correction values are stored in EEPROM
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





#define DEBUG


// easy way to set PWM outputs
#include <PWMFreak.h>


// Inputs and Outputs *************************************
// inputs
#define CLOCK_IN_PIN     2
#define RESET_IN_PIN     3
#define PLAY_REC_IN_PIN  4

// ADC (AVR name)
//#define CV_IN_PIN     PC5

// LEDs
#define PLAY_REC_LED_PIN  5
#define CLOCK_LED_PIN     6
#define RESET_LED_PIN     7

// PWM pins
#define PITCH_MSB_PIN   9   // D9
#define PITCH_LSB_PIN   10  // D10


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




// 16-bit values
const uint16_t test_values16[] = {
  /*
  C      C#     D      D#     E      F      F#     G      G#     A      A#     B
  */
  0,     1092,  2185,  3277,  4369,  5461,  6554,  7646,  8738,  9830,  10923, 12015,
  13107, 14199, 15292, 16384, 17476, 18569, 19661, 20753, 21845, 22938, 24030, 25122,
  26214, 27307, 28399, 29491, 30583, 31676, 32768, 33860, 34953, 36045, 37137, 38229,
  39322, 40414, 41506, 42598, 43691, 44783, 45875, 46967, 48060, 49152, 50244, 51337,
  52429, 53521, 54613, 55706, 56798, 57890, 58982, 60075, 61167, 62259, 63351, 64444,
  65535
};


int16_t correction_values[60];
int16_t octave_correction_values[5]; // 0 to 4, values are used to calculate each note correction values

uint8_t current_value_index;
uint8_t current_calibrated_octave;





#include <EEPROM.h>

#define EE_BASE_ADDR 54

uint16_t save_calibration(void);
uint16_t recall_calibration(void);




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


//  mode = Play;

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
    Serial.print(F("Calibration of CV Output for values 1 to 4V\n"));
  #endif

  current_value_index = 0;
  current_calibrated_octave = 0;

  recall_calibration();
  render_cv();
}


void render_cv(void)
{
  render_cv(test_values16[current_value_index] + octave_correction_values[current_calibrated_octave]);

  Serial.print(F("Render 16-bit value: "));
  Serial.print(test_values16[current_value_index]);
  Serial.print(F(" correction: "));
  Serial.print(octave_correction_values[current_calibrated_octave]);

  Serial.println("");
}

void render_cv(uint16_t cv)
{
  analogWrite(PITCH_MSB_PIN, (cv & 0xff00) >> 8);
  analogWrite(PITCH_LSB_PIN, cv & 0xff);
}


void loop()
{
  uint8_t trigger = false;

  myKeyDetector.detect();           // read keys and jack inputs
  trigger = myKeyDetector.trigger;  // store the name of the pressed key, if detected

  if (trigger)                // if a key has been pressed (or a rising edge on jack inputs)
  {
    switch (trigger)
    {
      case KEY_PLAY_REC:
        octave_correction_values[current_calibrated_octave] += 8;
        save_calibration();
        break;
      case KEY_STEP:              // button "Step" has been pressed or "Clock" jack input has received a rising edge
        //current_value_index++;
        current_calibrated_octave++;
        if (current_calibrated_octave > 4)
        {
          current_calibrated_octave = 0;
        }
        current_value_index = current_calibrated_octave * 12;
        break;
      case KEY_RESET:             // button "Reset" has been pressed or "Reset" jack input has received a rising edge
        octave_correction_values[current_calibrated_octave] -= 8;
        save_calibration();
        break;
      case KEY_NONE:
        break;
    }
    
    render_cv();
  }
}



uint16_t recall_calibration(void)
{
  uint16_t ee_address = EE_BASE_ADDR;
  int16_t value = 0;

  Serial.print(F("Recall calibration values: "));

  for (uint8_t i = 0; i < 4; i++)
  {
    EEPROM.get(ee_address, value);

    if (value == -1)    // eeprom data is -1 when not set
    {
      value = 0;
    }
    octave_correction_values[i + 1] = value;

    Serial.print(octave_correction_values[i + 1]);
    Serial.print(",");
    ee_address += sizeof(value);
  }

  Serial.print("\n");

  return ee_address;
}

uint16_t save_calibration(void)
{
  uint16_t ee_address = EE_BASE_ADDR;
  int16_t value = 0;

  Serial.print(F("Save calibration values: "));

  for (uint8_t i = 0; i < 4; i++)
  {
    value = octave_correction_values[i + 1];
    
    EEPROM.put(ee_address, value);

    Serial.print(octave_correction_values[i + 1]);
    Serial.print(",");
    ee_address += sizeof(value);
  }

  Serial.print("\n");

  return ee_address;
}
