// Edited and re_written by Tom Vajtay 4/2016
// Based off of V3_1 code.
// Designed to support two DRV2605L boards sharing the 0x5A address, address conflict is resolved through
// the use of an I2C multiplexer at 0x70.
// Also assuming MPR121 had address changed to 0x5D.
// Also there is code for pinouts to control an H-bridge to control the solenoid dispensers.


#include <MPR121.h>
#include <Wire.h>      //I2C Library
#include "Adafruit_DRV2605.h"  //Haptic Motor library
#define stim_effect1 47 //Edit this value to change the effect of first motor    (See Waveform Library Effects list for DRV2605L)
#define stim_effect2 47 //Edit this value to change the effect of the second motor
Adafruit_DRV2605 drv;   //Haptic motor intialization



char control_char;
char selector_char;

bool rew;
bool pun = true;
bool first;
bool autorew = false;
int count = 1;
int CamPin = 5;    //Sends TTL signal to camera
int SolDur = 500;        // Duration for opening Solenoid valve  -- 80msec: 2.5uL
int SolPin1 = 3;         // First input for H-bridge (Fresh Water)
int SolPin2 = 6;
int SolPin3 = 8;      // Second input for H-bridge (Salt Water)
int SolPin4 = 9;
int rewPer = 100;        // Period in ms of reward
int trials;
long firstlick;
long trigTime;
long stimTime;
int stimNum = 0;
int stimDur = 100;
long elapTime;
int lickOk = 1500;
bool trial_start = false;
bool random_toggle = false; //Default is non-random alternating stimulus activation
bool alternator = true;


void setup() {
  //declaring pin functions and starting TTL signal
  pinMode(SolPin1, OUTPUT);
  pinMode(SolPin2, OUTPUT);
  pinMode(SolPin3, OUTPUT);
  pinMode(SolPin4, OUTPUT);
  pinMode(CamPin, OUTPUT);
  digitalWrite(SolPin2, LOW);
  digitalWrite(SolPin4, LOW);
  digitalWrite(CamPin, LOW);

  //Uses analog signal from empty A3 to serve as random starting point for random number generation
  randomSeed(analogRead(3));

  // Start I2C
  Wire.begin();

  tcaselect(3);  //opens both TCA channels so both DRV boards get activated

  //Activates the DRV2605L board(s)
  drv.begin();
  drv.selectLibrary(1);

  //MPR121 activation sequence
  // 0x5D is the MPR121 I2C address
  if (!MPR121.begin(0x5D)) {
    Serial.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;
    }
    while (1);
  }
  MPR121.setInterruptPin(12);
  // this is the touch threshold - setting it low makes it more like a proximity trigger
  // default value is 40 for touch
  MPR121.setTouchThreshold(40);
  // this is the release threshold - must ALWAYS be smaller than the touch threshold
  // default value is 20 for touch
  MPR121.setReleaseThreshold(20);
  // initial data update
  MPR121.updateTouchData();

  //Serial Monitor Setup
  Serial.begin(9600);

  Serial.println("Behavioral Setup");
  Serial.println("Press 1 to start experiment");
  Serial.println("Press 2 to end session");
  Serial.println("Press 3 to flush-water");
  Serial.println("Press 4 to flush-saltwater");
  Serial.println("Press 5 for stim test");
  Serial.println("Press 6 to stop flush/stim test");
  Serial.println("Press 7 for autoreward, ON(1)/OFF(0)");
  Serial.println("Press 8 for 2 stimulus with punishment(1) OR 2 stimulus no punishment(0)");
  Serial.println("Press 9 to toggle either random 2 stimulus(75%/25%), or alternating 2 stimulus");
  delay(500);
}

void loop() {

  if (Serial.available()) {
    control_char = Serial.read();

    if (control_char == '9') {
      random_toggle = !random_toggle;
      Serial.print("Random:\t");
      Serial.println(random_toggle);
    }

    if (control_char == '7') {      //Dispenses reward amount of water
      autorew = !autorew;
      Serial.print("Auto Reward:\t");
      Serial.print("\t");
      Serial.println(autorew);
    }

    if (control_char == '8') {
      pun = !pun;
      Serial.print("Punishment:\t");
      Serial.print("\t");
      Serial.println(pun);
    }

    if (control_char == '3') {      //Flushes out the water line
      flush_1();
    }

    if (control_char == '4') {      //Flushes out the salt-water line
      flush_2();
    }

    if (control_char == '5') {      // Allows test of motors
      stim_test();
    }

    if (control_char == '2') {      //Stops the trial
      end_session();
    }

    if (control_char == '1') {      //Starts the trial
      Serial.println("How many trials do you want in this session?");
      Serial.println("Press b for 10 trials.");
      Serial.println("Press n for 30 trials.");
      Serial.println("Press m for 60 trials.");
    }

    if (control_char == 'b') {
      trials = 10;
      count = 1;
      Serial.println("Trial Begins");
      trial_start = true;
    }

    if (control_char == 'n') {
      trials = 30;
      count = 1;
      Serial.println("Trial Begins");
      trial_start = true;
    }

    if (control_char == 'm') {
      trials = 60;
      count = 1;
      Serial.println("Trial Begins");
      trial_start = true;
    }
  }


  if (trial_start) {
    Serial.println();
    Serial.println("Trial#\tTime\tEvent");
  }

  while (trial_start) {

    if (count <= trials) {              // Number of trials per a session


      if (Serial.available()) {
        control_char = Serial.read();


        if (control_char == '7') {      //Dispenses reward amount of water if lick is not detected in the reward time period
          autorew = !autorew;
          Serial.print("Auto Reward:\t");
          Serial.print("\t");
          Serial.println(autorew);
        }

        if (control_char == '8') {
          pun = !pun;
          Serial.print("Punishment:\t");
          Serial.print("\t");
          Serial.println(pun);
        }

        if (control_char == '3') {
          flush_1();
        }

        if (control_char == '4') {      //Flushes out the salt-water line
          flush_2();
        }

        if (control_char == '2') {      //Stops the trial
          end_session();
        }
      }

      if (pun) {
        elapTime = 0;
        first = false;
        rew = false;  //For autoreward activation, only turned true when reward lick detected which blocks autoreward
        trigTime = millis();
        digitalWrite(CamPin, HIGH);       // Sends TTL to Master  9 or camera controller

        early(2000);      // The function that detects an early lick in first two seconds of trial
        stimTime = millis();
        Serial.print(count);
        Serial.print("\t");
        Serial.print(stimTime - trigTime);
        if (random_toggle) {
          int randNum = random(4);

          if (randNum < 3) {
            stimNum = 1;
            Serial.println("\tStim 1");     //Stim 1 has a 75% chance of occuring, Stim 2 has a 25% chance of occuring
          }

          else if (randNum > 2) {
            stimNum = 2;
            Serial.println("\tStim 2");
          }
        }

        else if (!random_toggle) {
          if (alternator) {
            stimNum = 1;
            Serial.println("\tStim 1");
            alternator = !alternator;
          }
          else if (!alternator) {
            stimNum = 2;
            Serial.println("\tStim 2");
            alternator = !alternator;
          }
        }

        while (elapTime < lickOk) {     //For 1.5 seconds after stim mouse can lick sensor to recieve reward or punishment.
          if (elapTime < stimDur) {

            if (stimNum == 1) {
              //for stim1

              drv.setWaveform(0, stim_effect1);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              tcaselect(2);
              drv.go();
            }

            else if (stimNum == 2) {
              //for stim2

              drv.setWaveform(0, stim_effect2);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              tcaselect(1);
              drv.go();
            }
          }


          if (MPR121.touchStatusChanged()) {
            //read the touch state from the MPR121
            MPR121.updateTouchData();

            if (MPR121.isNewTouch(0)) {
              if (!first) {
                firstlick = millis() - trigTime;

                if (stimNum == 1) {       //Only first lick in the 1.5 timeframe will give reward or punishment
                  Serial.print(count);
                  Serial.print("\t");
                  Serial.print(firstlick);
                  Serial.println("\tReward Lick");
                  rew = true;
                  first = true;
                  reward();
                }

                else if (stimNum == 2) {
                  Serial.print(count);
                  Serial.print("\t");
                  Serial.print(firstlick);
                  Serial.println("\tPunishment Lick");
                  punishment();
                  first = true;
                }

              }
              else if (first & stimNum == 1) {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Good");
              }
              else if (first & stimNum == 2) {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Bad");
              }
            }
          }
          elapTime = millis() - stimTime; //Updates the time for the while loop
        }
      }

      if (!pun) {        //If there is no punishment or only one stimulus
        elapTime = 0;
        first = false;
        rew = false;  //For autoreward activation, only turned true when reward lick detected which blocks autoreward
        trigTime = millis();
        digitalWrite(CamPin, HIGH);       // Sends TTL to Master  9
        early(2000);          // The function that detects an early lick in first two seconds of trial
        stimTime = millis();
        stimNum = 1;
        Serial.print(count);
        Serial.print("\t");
        Serial.print(stimTime - trigTime);
        Serial.println("\tStim 1");


        if (random_toggle) {
          int randNum = random(4);

          if (randNum < 3) {
            stimNum = 1;
            Serial.println("\tStim 1");     //Stim 1 has a 75% chance of occuring, Stim 2 has a 25% chance of occuring
          }

          else if (randNum > 2) {
            stimNum = 2;
            Serial.println("\tStim 2");
          }
        }

        else if (!random_toggle) {
          if (alternator) {
            stimNum = 1;
            Serial.println("\tStim 1");
            alternator = !alternator;
          }
          else if (!alternator) {
            stimNum = 2;
            Serial.println("\tStim 2");
            alternator = !alternator;
          }
        }

        while (elapTime < lickOk) {     //For 1.5 seconds after stim mouse can lick sensor to recieve reward or punishment.
          if (elapTime < stimDur) {

            if (stimNum == 1) {
              //for stim1

              drv.setWaveform(0, stim_effect1);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              tcaselect(2);
              drv.go();
            }

            else if (stimNum == 2) {
              //for stim2

              drv.setWaveform(0, stim_effect2);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              tcaselect(1);
              drv.go();
            }
          }


          if (MPR121.touchStatusChanged()) {
            //read the touch state from the MPR121
            MPR121.updateTouchData();

            if (MPR121.isNewTouch(0)) {
              if (!first) {
                firstlick = millis() - trigTime;

                if (stimNum == 1) {                   //Only first lick in the 1.5 timeframe will give reaward or punishment
                  Serial.print(count);
                  Serial.print("\t");
                  Serial.print(firstlick);
                  Serial.println("\tReward Lick");
                  rew = true;
                  first = true;
                  reward();
                }

                else if (stimNum == 2) {
                  Serial.print(count);
                  Serial.print("\t");
                  Serial.print(firstlick);
                  Serial.println("\tBad Lick");
                  first = true;
                }
              }
              else if (first & stimNum == 1) {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Good");
              }
              else if (first & stimNum == 2) {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Bad");
              }
              elapTime = millis() - stimTime; //Updates the time for the while loop
            }


          }


          if (stimNum == 1) {     // If after 1.5 sec trial mouse did not lick for reward, reward given if autoreward is turned on Auto reward works with or without punishment
            if (autorew) {
              if (!rew) {
                reward();
              }
            }
          }

          late(3500);       //For 3.5 secs after LickOk period all licks will be recorded as late, no punishment or reward given.
          Serial.println("Trial End");
          count = count + 1;
          digitalWrite(CamPin, LOW);

          delay(3000);   //Inter Trial Interval of 4 secs
        }


        else {
          count = 1;
          Serial.println("Session Finished");
          trial_start = false; //Possibly redundant since end_session does a soft restart
          end_session();
        }
      }

    }



    void reward() {

      digitalWrite(SolPin1, HIGH);    // open solenoid valve for a short time
      delay(SolDur);                  // 8ms ~= 8uL of reward liquid (on box #4 011811)
      digitalWrite(SolPin1, LOW);
      Serial.print(count);
      Serial.print("\t");
      Serial.print(millis() - trigTime);
      Serial.println("\tWater Delivered");
    }

    void punishment() {

      digitalWrite(SolPin3, HIGH);    // open solenoid valve for a short time
      delay(SolDur);                  // 8ms ~= 8uL of punishment liquid
      digitalWrite(SolPin3, LOW);
      Serial.print(count);
      Serial.print("\t");
      Serial.print(millis() - trigTime);
      Serial.println("\tSalt Water Delivered");
    }


    void end_session() {
      Serial.println();
      delay(500);
      asm volatile ("  jmp 0");   // Makes the Arduino soft reset
    }

    void flush_1() {
      digitalWrite(SolPin1, HIGH);
      while (1) {
        if (Serial.available()) {
          char control_char = Serial.read();

          if (control_char == '6') {
            digitalWrite(SolPin1, LOW);
            Serial.println("Line Flushed");
            break;
          }
        }
      }
    }

    void flush_2() {
      digitalWrite(SolPin3, HIGH);
      while (1) {
        if (Serial.available()) {
          char control_char = Serial.read();

          if (control_char == '6') {
            digitalWrite(SolPin3, LOW);
            Serial.println("Line Flushed");
            break;
          }
        }
      }
    }

    void early(int dur) {
      long start = millis();
      while ((millis() - start) < dur) {    //While time passed between start and current time is less than dur
        if (MPR121.touchStatusChanged()) {
          //read the touch state from the MPR121
          MPR121.updateTouchData();

          if (MPR121.isNewTouch(0)) {        //If touched during early period print lick early
            Serial.print(count);
            Serial.print("\t");
            Serial.print(millis() - trigTime);
            Serial.println("\tLick Early");
          }
        }
      }
    }

    void late(int dur) {
      long temp = millis();
      while ((millis() - temp) < dur) {
        if (MPR121.touchStatusChanged()) {
          //read the touch state from the MPR121
          MPR121.updateTouchData();

          if (MPR121.isNewTouch(0)) {
            Serial.print(count);
            Serial.print("\t");
            Serial.print(millis() - trigTime);
            Serial.println("\tLick Late");
          }
        }
      }
    }

    void stim_test() {
      Serial.println("Stim test start");
      while (1) {
        tcaselect(1);
        drv.setWaveform(0, stim_effect1);  // the effect
        drv.setWaveform(1, 0);   // end waveform
        drv.go();
        delay(500);

        tcaselect(2);
        drv.setWaveform(0, stim_effect2);  // the effect
        drv.setWaveform(1, 0);   // end waveform
        drv.go();
        delay(500);

        if (Serial.available()) {
          char control_char = Serial.read();
          if (control_char == '6') {
            Serial.println("End");
            break;
          }
        }
      }

    }

    void tcaselect(uint8_t i) {
      if (i > 3) return;

      Wire.beginTransmission(0x70);
      Wire.write(i);
      Wire.endTransmission();
    }
