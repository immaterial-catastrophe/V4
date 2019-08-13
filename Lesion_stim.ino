//Tom Vajtay revisions 08/2019

#include <Wire.h>      //I2C Library
#include "Adafruit_DRV2605.h"  //Haptic Motor library
#define stim_effect1 47 //Edit this value to change the effect of first motor    (See Waveform Library Effects list for DRV2605L)
#define stim_effect2 47 //Edit this value to change the effect of the second motor

Adafruit_DRV2605 drv;   //Haptic motor intialization

#define CAMERA_PIN             5        


//Definitions for Switch/Case
#define LESION            0
/*--------------------------
  variables
  --------------------------*/

char control_char;
int count = 0;

unsigned long timestamp;
bool onSignal = false;

int single_pulse_count = 0;

int pulse_count = 0;
int t_duration = 5; // longer than 10msec
int dur_count = 0;
int signalChange = 0;
int loop_count = 0;
int ITI = 5000;  //Inter Trial Interval, time between recording of trials in ms
          /*-------------------------
            functions
            -------------------------*/

void setup() {

  pinMode(CAMERA_PIN, OUTPUT);

  digitalWrite(CAMERA_PIN, LOW);

  Wire.begin();

  tcaselect(3);  //opens both TCA channels so both DRV boards get activated

  //Activates the DRV2605L board(s)
  drv.begin();
  drv.selectLibrary(1);

  Serial.begin(9600);

  Serial.println("Enter s to start whisker stim (x50)");
  Serial.println("You can enter q(quit) or p(pause) during recording");

  onSignal = false;

}


void loop() {

  while (Serial.available()) {
    control_char = Serial.read();


    if (control_char == 's') {
      onSignal = true;
      Serial.println("Starting whisker stim, 10s x 50 trials");
      timestamp = millis();
      signalChange = 0;
      count = 0;
    }
    else if (control_char == 'q') {
      digitalWrite(CAMERA_PIN, LOW);
      onSignal = false;
      count = 0;
      delay(500);
      asm volatile ("  jmp 0");   // Makes the Arduino soft reset
    }
    else {
      Serial.println("Enter s");
    }
  }

  while (onSignal) {   //During the experiment this part loops for every unique trial allowing for pause/quit in the middle of a run
    e_stop();

    switch (signalChange) {
      case LESION:
        Serial.println();
        Serial.println("Trial\tStim\tFinish");
        while (count < 50) {
            Serial.print(count + 1);
            Serial.print("\t");
            motor_pulse();
            iti_delay(ITI);
            count++;
        }
        onSignal = 0;
        count = 0;
        break;
    }//end of case
    delay(500);
    asm volatile ("  jmp 0");   // Makes the Arduino soft reset at the end of experiment
  }//end of while
}//end of loop()

void motor_pulse() {
  double delay_start = millis();
  bool flag = true;
  drv.setWaveform(0, stim_effect1);  // the effect
  drv.setWaveform(1, 0);   // end waveform
  digitalWrite(CAMERA_PIN, HIGH);
  tcaselect(2);
  while (millis() - delay_start < 10000) {
    if(millis() - delay_start > 200){
      digitalWrite(CAMERA_PIN, LOW);
    }
      
    e_stop();
    if (flag && millis() - delay_start > 2000) {
      Serial.print(int(millis() - delay_start));
      drv.go();
      flag = false;
    }
  }
  digitalWrite(CAMERA_PIN, HIGH);
  Serial.print("\t");
  Serial.println(int(millis() - delay_start));
  iti_delay(200);
  digitalWrite(CAMERA_PIN, LOW);
}

void iti_delay(int duration) {
  double ITI_start = millis();
  while (millis() - ITI_start < duration ){
    e_stop();
  }
}

void e_stop() {
  if (Serial.available()) {
    control_char = Serial.read();
    if (control_char == 'q') {
      digitalWrite(CAMERA_PIN, LOW);
      onSignal = false;
      Serial.println("Trigger function stopped");
      count = 0;
      delay(500);
      asm volatile ("  jmp 0");   // Makes the Arduino soft reset
    }
    else if (control_char == 'p') {
      digitalWrite(CAMERA_PIN, LOW);
      double pause_start = millis();
      Serial.println("Trigger function paused, enter 'r' to resume");
      while (Serial.available()) {
        control_char = Serial.read();
        if (control_char == 'r') {
          Serial.print("Paused for ");
          Serial.print((pause_start - millis())/1000);
          Serial.println(" seconds");
          break;
        }
        else {
          Serial.println("That wasn't 'r' ");
        }
      } // while for resume
    } //control_char if, else if
  } // If Serial.available
} //Function end

void tcaselect(uint8_t i) {
  if (i > 3) return;

  Wire.beginTransmission(0x70);
  Wire.write(i);
  Wire.endTransmission();
}
