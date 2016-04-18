// Edited and re_written by Tom Vajtay 3/2016
// Based off of V3_1 code.
// Designed to support two DRV2605L boards sharing the 0x5A address assuming their outputs are gated with transistors.
// Also assuming MPR121 had address changed to 0x5D.
// And pinouts to control an H-bridge to power the solenoid dispenser.

#include <mpr121.h>		//Capacitive sensor header file ***Do not use a source code, registers declared later in program***
#include <Wire.h>	   	//I2C Library
#include "Adafruit_DRV2605.h"  //Haptic Motor library
Adafruit_DRV2605 drv;  	//Haptic motor

//byte control_char;
char control_char;
float a = 1;
float b = 100;
int x = 6;
int y = 9;
double probability;

bool rew;
bool first;
bool timeout;
bool autorew = false;
int limit = 1;
int count = 0;
int timecount = 0;

int CamPin = 5;		 //Sends TTL signal to camera
int SolDur = 500;        // Duration for opening Solenoid valve  -- 80msec: 2.5uL
int SolPin1 = 3;         // First input for H-bridge
int SolPin2 = 6; 	 // Second input for H-bridge
int stimPin = 10;        // Positive stimulus
int stimPin2 = 11;       // Negative stimulus
int irqpin = 12;          // Touch sensor interrupt pin
int rewPer = 100;        // Period in ms of reward
bool newData = false;    // Bool licked?
long hperiod = 100;
long time = 0;
long firstlick;
long trigTime;
long stimTime;
long stimNum;
int stimDur = 100;
long elapTime;
int lickOk = 1500;
int drinkDur = 5000;
bool led;
bool trial_start = false;

int punishmentFlag = 0;
int rewardFlag = 0;
int afterlick = 0;


void setup() {
  //declaring pin functions and starting TTL signal
  pinMode(SolPin1, OUTPUT);
  pinMode(SolPin2, OUTPUT);
  pinMode(irqpin, INPUT);
  pinMode(CamPin, OUTPUT);
  pinMode(stimPin, OUTPUT);
  pinMode(stimPin2, OUTPUT);
  digitalWrite(irqpin, HIGH);
  digitalWrite(SolPin2, LOW);
  digitalWrite(stimPin, LOW);
  digitalWrite(stimPin2, LOW);

  //Uses analog signal from empty A3 to serve as random starting point for random number generation
  randomSeed(analogRead(3));

  // Start I2C
  Wire.begin();

  // Set the registers on the capacitive sensing IC
  setupCapacitiveRegisters();
  Serial.begin(9600);

  //Serial Monitor Setup
  Serial.println("Behavioral Setup");
  Serial.println("Press s to start stimulation task");
  Serial.println("Press p to stop stimulation task");
  Serial.println("Press f to flush-water");
  Serial.println("Press f1 to flush-saltwater");
  Serial.println("Press x to stop flush");
  Serial.println("Press a for autoreward");
  Serial.print("Probabilty:\t");
  Serial.print("\t");
  probability = (x / (y + 1));
  Serial.println(probability, DEC);
  delay(500);
  Serial.println("Trial#\tTime\tEvent");
}


void loop() {

  if (Serial.available()) {
    control_char = Serial.read();

    if (control_char == 'a') {			//Dispenses reward amount of water
      autorew = !autorew;
      Serial.print("Auto Reward:\t");
      Serial.print("\t");
      Serial.println(autorew);
    }

    if (control_char == 'f') {			//Flushes out the water line
      flush_1();
    }

	if (control_char == 'f1') {			//Flushes out the salt-water line
      flush_2();
    }
	
    if (control_char == 'p') {			//Stops the trial
      end_trial();
    }

    if (control_char == 's') {			//Starts the trial
      count = 0;
      Serial.println("Trial Begins");
      trial_start = true;
    }
  }

  while (trial_start) {

    if (count <= 124) {
      digitalWrite(CamPin, HIGH);		//Starts camera


      if (Serial.available()) {
        control_char = Serial.read();
      }

      if (control_char == 'a') {			//Dispenses reward amount of water
        autorew = !autorew;
        Serial.print("Auto Reward:\t");
        Serial.print("\t");
        Serial.println(autorew);
      }

      if (control_char == 'f') {
        flush_1();
      }
	  
	  if (control_char == 'f1') {			//Flushes out the salt-water line
      flush_2();
	  }

      if (control_char == 'p') {			//Stops the trial
        end_trial();
      }

      elapTime = 0;
      led = true;
      first = false;
      timeout = false;
      rew = false;
      trigTime = millis();
      time = trigTime;

      del1(2000);
      stimTime = millis();

      Serial.print(count);
      Serial.print("\t");
      Serial.print(stimTime - trigTime);

      int randNum = random(0, y);

      if (randNum <= 3) {
        stimNum = 0;
        Serial.println("\tNoStim");
      }

      else if (randNum > 3 && randNum <= 6) {
        stimNum = 1;
        Serial.println("\tStim1");
      }

      else {
        stimNum = 2;
        Serial.println("\tStim2");
      }

      while (elapTime < lickOk) {
        if (!first) {
          if (elapTime < stimDur) {
            if (stimNum == 0) {
              digitalWrite(stimPin, LOW);
              digitalWrite(stimPin2, LOW);
            }

            else if (stimNum == 1) {
              //for stim1
              digitalWrite(stimPin, HIGH);
              digitalWrite(stimPin2, LOW);
              drv.setWaveform(0, 47);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              delay(100);
              drv.go();
            }

            else if (stimNum == 2) {
              //for stim2
              digitalWrite(stimPin2, HIGH);
              digitalWrite(stimPin, LOW);
              drv.setWaveform(0, 47);  // the effect
              drv.setWaveform(1, 0);   // end waveform
              delay(100);
              drv.go();
            }

            else {

              digitalWrite(stimPin, LOW);
              digitalWrite(stimPin2, LOW);
            }
          }
        }

        if (!checkInterrupt()) {
          //read the touch state from the MPR121
          Wire.requestFrom(0x5D, 2);

          byte LSB = Wire.read();
          byte MSB = Wire.read();

          uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states

          if (touched & (1)) {
            if (!first) {
              firstlick = millis() - trigTime;

              if (stimNum == 1) {
                digitalWrite(stimPin, LOW);
                digitalWrite(stimPin2, LOW);
                Serial.print(count);
                Serial.print("\t");
                Serial.print(firstlick);
                Serial.println("\tReward Lick");
                rew = true;
                first = true;
                reward();
              }

              else if (stimNum == 0) {
                digitalWrite(stimPin, LOW);
                digitalWrite(stimPin2, LOW);
                Serial.print(count);
                Serial.print("\t");
                Serial.print(firstlick);
                Serial.println("\tTimeout Lick");
                timeout = true;
                first = true;
              }

            }
            else {
              if (timeout) {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Bad");
                afterlick = 1;
              }
              else {
                Serial.print(count);
                Serial.print("\t");
                Serial.print(millis() - trigTime);
                Serial.println("\tLick Good");
                afterlick = 1;
              }
            }
          }
        }

        elapTime = millis() - stimTime;
      }
      if (stimNum == 1) {
        if (autorew) {
          if (!rew) {
            reward();
          }
        }
      }

      del3(2000);

      Serial.println("Trial End");
      count = count + 1;
      if (timeout) {
        delay(5000);
      }
      delay(500 + random(200, 500));
    }

    else {
      count = 0;
      Serial.println("Session Finished");
      trial_start = false;
      end_trial();
    }

  }

}

void reward() {

  rewardFlag = 1;
  digitalWrite(SolPin1, HIGH);    // open solenoid valve for a short time
  delay(SolDur);                  // 8ms ~= 8uL of reward liquid (on box #4 011811)
  digitalWrite(SolPin1, LOW);
}

void punishment() {
  

  // PUNSIHMENT SEQUENCE  
  // go through reward/vacuum solenoid sequence
  punishmentFlag = 1;
  digitalWrite(SolPin3, HIGH);    // open solenoid valve for a short time
  delay(350);                  // 8ms ~= 8uL of reward liquid (on box #4 011811)
  digitalWrite(SolPin3, LOW);
  Serial.print(count);
  Serial.print("\t");
  Serial.print(millis()-trigTime);
  Serial.println("\tSalt Water Not Delivered");


  //elapTime = stimDur + 1;  // break out of the reward stimulus loop after receiving reward

}

void end_trial() {
  while (1) {
    if (Serial.available()) {
      byte control_char = Serial.read();

      if (control_char == 'r') {
        count = 0;
        break;
      }
    }
  }
}

void flush_1() {
  digitalWrite(SolPin1, HIGH);
  while (1) {
    if (Serial.available()) {
      byte control_char = Serial.read();

      if (control_char == 'x') {
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
      byte control_char = Serial.read();

      if (control_char == 'x') {
        digitalWrite(SolPin3, LOW);
        Serial.println("Line Flushed");
        break;
      }
    }
  }
}

void del1(int dur) {
  long temp = millis();
  while ((millis() - temp) < dur) {
    if (!checkInterrupt()) {
      Wire.requestFrom(0x5D, 2);

      byte LSB = Wire.read();
      byte MSB = Wire.read();

      uint16_t touched = ((MSB << 8) | LSB);

      if (touched & (1)) {
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis() - trigTime);
        Serial.println("\tLick Early");
      }
    }
  }
}

void del3(int dur) {
  long temp = millis();
  while ((millis() - temp) < dur) {
    if (!checkInterrupt()) {
      //read the touch state from the MPR121
      Wire.requestFrom(0x5D, 2);

      byte LSB = Wire.read();
      byte MSB = Wire.read();

      uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


      // Check what electrodes were pressed
      if (touched & (1)) {
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis() - trigTime);
        Serial.println("\tLick Late");
        afterlick = 1;
      }
    }
  }
}


void setupCapacitiveRegisters(void) {

  set_register(0x5D, ELE_CFG, 0x00);

  // Section A - Controls filtering when data is > baseline.
  set_register(0x5D, MHD_R, 0x01);
  set_register(0x5D, NHD_R, 0x01);
  set_register(0x5D, NCL_R, 0x00);
  set_register(0x5D, FDL_R, 0x00);

  // Section B - Controls filtering when data is < baseline.
  set_register(0x5D, MHD_F, 0x01);
  set_register(0x5D, NHD_F, 0x01);
  set_register(0x5D, NCL_F, 0xFF);
  set_register(0x5D, FDL_F, 0x02);

  // Section C - Sets touch and release thresholds for each electrode
  set_register(0x5D, ELE0_T, TOU_THRESH);
  set_register(0x5D, ELE0_R, REL_THRESH);

  set_register(0x5D, ELE1_T, TOU_THRESH);
  set_register(0x5D, ELE1_R, REL_THRESH);

  set_register(0x5D, ELE2_T, TOU_THRESH);
  set_register(0x5D, ELE2_R, REL_THRESH);

  set_register(0x5D, ELE3_T, TOU_THRESH);
  set_register(0x5D, ELE3_R, REL_THRESH);

  set_register(0x5D, ELE4_T, TOU_THRESH);
  set_register(0x5D, ELE4_R, REL_THRESH);

  set_register(0x5D, ELE5_T, TOU_THRESH);
  set_register(0x5D, ELE5_R, REL_THRESH);

  set_register(0x5D, ELE6_T, TOU_THRESH);
  set_register(0x5D, ELE6_R, REL_THRESH);

  set_register(0x5D, ELE7_T, TOU_THRESH);
  set_register(0x5D, ELE7_R, REL_THRESH);

  set_register(0x5D, ELE8_T, TOU_THRESH);
  set_register(0x5D, ELE8_R, REL_THRESH);

  set_register(0x5D, ELE9_T, TOU_THRESH);
  set_register(0x5D, ELE9_R, REL_THRESH);

  set_register(0x5D, ELE10_T, TOU_THRESH);
  set_register(0x5D, ELE10_R, REL_THRESH);

  set_register(0x5D, ELE11_T, TOU_THRESH);
  set_register(0x5D, ELE11_R, REL_THRESH);

  // Section D
  // Set the Filter Configuration
  // Set ESI2
  set_register(0x5D, FIL_CFG, 0x04);

  // Section E
  // Electrode Configuration
  // Set ELE_CFG to 0x00 to return to standby mode
  set_register(0x5D, ELE_CFG, 0x0C);  // Enables all 12 Electrodes


  // Section F
  // Enable Auto Config and auto Reconfig
  /*set_register(0x5D, ATO_CFG0, 0x0B);
   set_register(0x5D, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5D, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
   set_register(0x5D, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V

  set_register(0x5D, ELE_CFG, 0x0C);

}

/**
 * set_register Sets a register on a device connected via I2C. It accepts the device's address,
 *   register location, and the register value.
 * @param address The address of the I2C device
 * @param r       The register's address on the I2C device
 * @param v       The new value for the register
 */

void set_register(int address, unsigned char r, unsigned char v) {
  Wire.beginTransmission(address);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}

boolean checkInterrupt(void) {
  //Serial.println(digitalRead(irqpin));
  return digitalRead(irqpin);
}


