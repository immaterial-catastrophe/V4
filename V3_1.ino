
#include <Wire.h>
#include "mpr121.h"
#define PIN_GLOBAL_SYNC        5       // starts board 1, and sends out the signal to the board 2

//byte control_char;
char control_char;
float a=1;
float b=100;
int x=6;
int y=9;
double probability;

bool rew;
bool first;
bool timeout;
bool autorew=false;
int limit=1;
int count=0;
int timecount = 0;

int tonepin=7;
int CamPin=5;
//int puffpin=8;
int SolDur = 500;      // Duration for opening Solenoid valve  -- 80msec: 2.5uL
int SolPin1 = 3;      // Control pin for Solenoid
int SolPin2 = 6; 
int SolPin3 = 8; 
int SolPin4 = 9;
int stimPin=10;        // Pin for led stimulation
int stimPin2=11;
int irqpin = 2;        // Touch sensor interrupt pin
int rewPer=100;        // Period in ms of reward 
bool newData = false;  // Bool licked?
long hperiod=100; 
int intensity;
int ointensity;
long time=0;
long firstlick;
long trigTime;
long stimTime;
long stimNum;
long elapTime;
int stimDur=100;
int lickOk=1500;
int drinkDur=5000;
bool led;
bool trial_start = false;

int punishmentFlag = 0;
int rewardFlag = 0;
int afterlick = 0;

void setup(){
   //attachInterrupt(1,printlick,FALLING);  
   //pinMode(puffpin, OUTPUT);
   //digitalWrite(puffpin,LOW);
   pinMode(PIN_GLOBAL_SYNC, OUTPUT);
   digitalWrite(PIN_GLOBAL_SYNC, LOW);
   pinMode(SolPin1, OUTPUT);  // We only use SolPin1
   pinMode(SolPin2, OUTPUT);
   digitalWrite(SolPin2,LOW);
   pinMode(SolPin3, OUTPUT);  // We only use SolPin1
   pinMode(SolPin4, OUTPUT);
   digitalWrite(SolPin4,LOW);
   pinMode(irqpin,INPUT);
   digitalWrite(irqpin,HIGH);\
   pinMode(CamPin,OUTPUT);
   pinMode(stimPin,OUTPUT);
   pinMode(stimPin2,OUTPUT);
   digitalWrite(stimPin, LOW);
   digitalWrite(stimPin2, LOW);
   
   //attachInterrupt(0, touchSensor, FALLING);   // Adding Interrupt 1 on pin 2 on Arduino
   randomSeed(analogRead(3));
   
   // Set up I2C connection with MPR121 sensor
   Wire.begin();
   
   // Set the registers on the capacitive sensing IC
   setupCapacitiveRegisters();
   Serial.begin(9600);
   
  //Serial Monitor Setup 
  Serial.println("Behavioral Setup"); 
  Serial.println("Press s to start stimulation task");
  Serial.println("Press p to stop stimulation task");
  Serial.println("Press f to flush-water");
  Serial.println("Press g to flush-water");
  Serial.println("Press a for autoreward");
  Serial.print("Probabilty:\t");
      Serial.print("\t");
      probability=(x/(y+1));
      Serial.println(probability,DEC);
      delay(500);
      //Serial.print("Trial #");
      //Serial.print("\t");
      //Serial.println("Trial # \tTrial Type\tOutcome");\\
      Serial.println("Trial#\tTime\tEvent");
} 

void loop(){
  
  if(Serial.available()>0) {
    control_char=Serial.read();
    
    if (control_char =='a') {
      autorew=!autorew;
      Serial.print("Auto Reward:\t");
      Serial.print("\t");
      Serial.println(autorew);
    }
    if (control_char == 'f'){
      flush();
    }
    if (control_char == 'g'){
      flush_2();
    }
    if (control_char=='p') {
      end();
    }
    if (control_char =='s') {
      count = 0;
      Serial.println("Trial Begins");
      trial_start = true;
    }
  }
    
     while (trial_start){
       if (count <=124) {
       digitalWrite(CamPin,HIGH);
   
       
//        Serial.print(count);
//        Serial.print("\t");

      if(Serial.available()>0) {
          control_char=Serial.read();

      if (control_char =='a') {
        autorew=!autorew;
        Serial.print("Auto Reward:\t");
        Serial.print("\t");
        Serial.println(autorew);
      }if (control_char == 'f'){
        flush();
      }if (control_char=='p') {
        end();
      }
    }
//      
        elapTime = 0;
        led=true;
        first=false;
        timeout=false;
        rew=false;
        trigTime = millis();
        time=trigTime;
        
        del1(2000);
        stimTime=millis();
       //test@!!
       //hperiod=10;
       Serial.print(count);
       Serial.print("\t");
       Serial.print(stimTime-trigTime);
       
       int randNum = random(0, y);
       
        if (randNum <= y-x ) {
          stimNum = 0;
          Serial.println("\tNoStim");
        } 
        
        else if (randNum > y-x) {{ 
           stimNum = 1;
           Serial.println("\tStim1");
         }
//         else if (randNum > y+1){
//           stimNum = 2;
//           Serial.println("\tStim2");
//         }
        }      
     //Serial.print("Count:");
     //Serial.println(count);
   if (stimNum ==1){
     stimDur=100;
   }
//  else if (stimNum ==2){
//   stimDur = 100;
//  } 
   while (elapTime<lickOk) {
       if (!first) {
         if (elapTime < stimDur) {
           if (stimNum == 0){
             
             //for no stimulation
           
             //digitalWrite(stimPin,HIGH);
            
           }
           else if (stimNum == 1) {
             //for stim1
             digitalWrite(stimPin,HIGH);
        
           }
//           else if (stimNum == 2){
//             //for stim2
//            digitalWrite(stimPin2,HIGH);  
//            
//           }
          }else{
       
            digitalWrite(stimPin, LOW);
            digitalWrite(stimPin2,LOW);
           }
         }
      // if lick, is it correct or incorrect (based upon rewPos)
      if (!checkInterrupt()) { 
        
      //read the touch state from the MPR121 
      Wire.requestFrom(0x5A,2); 
  
      byte LSB = Wire.read();
      byte MSB = Wire.read();
  
      uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states
  
  
       // Check what electrodes were pressed
        if(touched & (1)){
          if (!first) {
            
            firstlick=millis()-trigTime;
            if (stimNum == 1) {
              digitalWrite(stimPin,LOW);
              Serial.print(count);
              Serial.print("\t");
              Serial.print(firstlick);
              Serial.println("\tReward Lick");
              noTone(tonepin);
              rew=true;
              first=true;
              reward();
              //del2g(drinkDur);
            }
            else if (stimNum == 0) {
             digitalWrite(stimPin,LOW);
             Serial.print(count);
             Serial.print("\t");
             Serial.print(firstlick);
             Serial.println("\tTimeout Lick");
             noTone(tonepin);
             tone(tonepin,5000,2000);
             punishment();
             timeout=true;
             first=true;
             //digitalWrite(puffpin,HIGH);
             //delay(SolDur);
             //digitalWrite(puffpin,LOW);
             //del2b(5000+SolDur);
            }  
          } 
         else {
          if (timeout) {
            Serial.print(count);
            Serial.print("\t");
            Serial.print(millis()-trigTime);
            Serial.println("\tLick Bad");
            afterlick = 1;
           }
          else {
            Serial.print(count);
            Serial.print("\t");
            Serial.print(millis()-trigTime);
            Serial.println("\tLick Good");
            afterlick = 1;
          }
         }
        }
       } 
    elapTime = millis() - stimTime;
    //Serial.println(elapTime);
     }
     
 if(stimNum == 1){
   if(autorew){
     if(!rew){
       reward();
   }
  }
 }
  //Serial.print("End LickOK: ");
  //Serial.println(millis());
  //digitalWrite(stimPin,LOW);
  del3(2000);
  /*
  if (rewardFlag && !afterlick){
    rewardFlag = 0;
    digitalWrite(SolPin2, HIGH);
    delay(SolDur);
    digitalWrite(SolPin2, LOW);
  }
  if (punishmentFlag && !afterlick){
    punishmentFlag = 0;
    digitalWrite(SolPin4, HIGH);
    delay(350);
    digitalWrite(SolPin4, LOW);
  }*/
  
  //Serial.println("Inter trial delay");
  //Serial.println();
  //digitalWrite(CamPin,LOW);
  //digitalWrite(SolPin2, HIGH);
  //digitalWrite(SolPin4, HIGH);
  //delay(SolDur);
  //digitalWrite(SolPin2, LOW);
  //digitalWrite(SolPin4, LOW);
  Serial.println("Trial End");
  count=count+1;
  if (timeout) {
    delay(5000);
  }
  delay(500+random(200,500)); // inter-trial delay
  
 }else{
  
    count = 0;
    Serial.println("Session Finished");
    trial_start = false;
    end();
    }
  
  
  }
     
}

// function for reward administration
void reward() {
  

  // REWARD SEQUENCE
  // go through reward/vacuum solenoid sequence
  rewardFlag = 1;
  digitalWrite(SolPin1, HIGH);    // open solenoid valve for a short time
  delay(SolDur);                  // 8ms ~= 8uL of reward liquid (on box #4 011811)
  digitalWrite(SolPin1, LOW);
//  Serial.print(count);
//  Serial.print("\t");
//  Serial.print(millis()-trigTime);
//  Serial.println("\tWater Delivered");


  //elapTime = stimDur + 1;  // break out of the reward stimulus loop after receiving reward

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
void end() {
  while (1) {
    if (Serial.available() > 0) {
      //Serial.println("new loop");
      // read the oldest byte in the serial buffer:
      byte control_char = Serial.read();
      // if it's a capital H (ASCII 72), turn on the LED:
      if (control_char == 'c') {
        break;
      }
      if (control_char == 'r') {
        count=0;
        break;
      }
      if (control_char == 'e') {
        echo();
      }
      if (control_char == 'f') {
        flush();
      }
    }
  }
}

void flush() {
  digitalWrite(SolPin1,HIGH);
  while (1) {
    if (Serial.available() > 0) {
      //Serial.println("new loop");
      // read the oldest byte in the serial buffer:
      byte control_char = Serial.read();
      // if it's a capital H (ASCII 72), turn on the LED:
      if (control_char == 'x') {
        digitalWrite(SolPin1,LOW);
        Serial.println("valve flushed");
        break;
      }
    }
  }
}
void flush_2() {
  digitalWrite(SolPin3,HIGH);
  while (1) {
    if (Serial.available() > 0) {
      //Serial.println("new loop");
      // read the oldest byte in the serial buffer:
      byte control_char = Serial.read();
      // if it's a capital H (ASCII 72), turn on the LED:
      if (control_char == 'g') {
        digitalWrite(SolPin3,LOW);
        Serial.println("salt water valve flushed");
        break;
      }
    }
  }
}

void echo() {
  if (Serial.available()) {
    Serial.write(Serial.read());
  }
}
void del1(int dur) {
  long temp=millis();
  while ((millis()-temp)<dur) {
    if (!checkInterrupt()) { 
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 

    byte LSB = Wire.read();
    byte MSB = Wire.read();

    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


     // Check what electrodes were pressed
      if(touched & (1)){
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis()-trigTime);
        Serial.println("\tLick Early");
        //tone(tonepin,5000,300);
        //intensity=0;
        //tone(tonepin,5000,8000);
        //delay(trigTime-millis()+2000+lickOk);
        //elapTime=9999;
  }
}
  }
}

void del2g(int dur) {
  long temp=millis();
  while ((millis()-temp)<dur) {
    if (!checkInterrupt()) { 
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 

    byte LSB = Wire.read();
    byte MSB = Wire.read();

    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


     // Check what electrodes were pressed
      if(touched & (1)){
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis()-trigTime);
        Serial.println("\tLick Good");
        afterlick = 1;
  }
}
  }
}


void del2b(int dur) {
  long temp=millis();
  while ((millis()-temp)<dur) {
    if (!checkInterrupt()) { 
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 

    byte LSB = Wire.read();
    byte MSB = Wire.read();

    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


     // Check what electrodes were pressed
      if(touched & (1)){
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis()-trigTime);
        Serial.println("\tLick Bad");
        afterlick =1;
  }
}
  }
}


void del3(int dur) {
  long temp=millis();
  while ((millis()-temp)<dur) {
    if (!checkInterrupt()) { 
    //read the touch state from the MPR121
    Wire.requestFrom(0x5A,2); 

    byte LSB = Wire.read();
    byte MSB = Wire.read();

    uint16_t touched = ((MSB << 8) | LSB); //16bits that make up the touch states


     // Check what electrodes were pressed
      if(touched & (1)){
        Serial.print(count);
        Serial.print("\t");
        Serial.print(millis()-trigTime);
        Serial.println("\tLick Late");
        afterlick =1;
  }
}
  }
}

void setupCapacitiveRegisters(void){

 set_register(0x5A, ELE_CFG, 0x00);

  // Section A - Controls filtering when data is > baseline.
  set_register(0x5A, MHD_R, 0x01);
  set_register(0x5A, NHD_R, 0x01);
  set_register(0x5A, NCL_R, 0x00);
  set_register(0x5A, FDL_R, 0x00);

  // Section B - Controls filtering when data is < baseline.
  set_register(0x5A, MHD_F, 0x01);
  set_register(0x5A, NHD_F, 0x01);
  set_register(0x5A, NCL_F, 0xFF);
  set_register(0x5A, FDL_F, 0x02);

  // Section C - Sets touch and release thresholds for each electrode
  set_register(0x5A, ELE0_T, TOU_THRESH);
  set_register(0x5A, ELE0_R, REL_THRESH);

  set_register(0x5A, ELE1_T, TOU_THRESH);
  set_register(0x5A, ELE1_R, REL_THRESH);

  set_register(0x5A, ELE2_T, TOU_THRESH);
  set_register(0x5A, ELE2_R, REL_THRESH);

  set_register(0x5A, ELE3_T, TOU_THRESH);
  set_register(0x5A, ELE3_R, REL_THRESH);

  set_register(0x5A, ELE4_T, TOU_THRESH);
  set_register(0x5A, ELE4_R, REL_THRESH);

  set_register(0x5A, ELE5_T, TOU_THRESH);
  set_register(0x5A, ELE5_R, REL_THRESH);

  set_register(0x5A, ELE6_T, TOU_THRESH);
  set_register(0x5A, ELE6_R, REL_THRESH);

  set_register(0x5A, ELE7_T, TOU_THRESH);
  set_register(0x5A, ELE7_R, REL_THRESH);

  set_register(0x5A, ELE8_T, TOU_THRESH);
  set_register(0x5A, ELE8_R, REL_THRESH);

  set_register(0x5A, ELE9_T, TOU_THRESH);
  set_register(0x5A, ELE9_R, REL_THRESH);

  set_register(0x5A, ELE10_T, TOU_THRESH);
  set_register(0x5A, ELE10_R, REL_THRESH);

  set_register(0x5A, ELE11_T, TOU_THRESH);
  set_register(0x5A, ELE11_R, REL_THRESH);

  // Section D
  // Set the Filter Configuration
  // Set ESI2
  set_register(0x5A, FIL_CFG, 0x04);

  // Section E
  // Electrode Configuration
  // Set ELE_CFG to 0x00 to return to standby mode
  set_register(0x5A, ELE_CFG, 0x0C);  // Enables all 12 Electrodes


  // Section F
  // Enable Auto Config and auto Reconfig
  /*set_register(0x5A, ATO_CFG0, 0x0B);
   set_register(0x5A, ATO_CFGU, 0xC9);  // USL = (Vdd-0.7)/vdd*256 = 0xC9 @3.3V   set_register(0x5A, ATO_CFGL, 0x82);  // LSL = 0.65*USL = 0x82 @3.3V
   set_register(0x5A, ATO_CFGT, 0xB5);*/  // Target = 0.9*USL = 0xB5 @3.3V

  set_register(0x5A, ELE_CFG, 0x0C);

}

/**
 * set_register Sets a register on a device connected via I2C. It accepts the device's address, 
 *   register location, and the register value.
 * @param address The address of the I2C device
 * @param r       The register's address on the I2C device
 * @param v       The new value for the register
 */
void set_register(int address, unsigned char r, unsigned char v){
  Wire.beginTransmission(address);
  Wire.write(r);
  Wire.write(v);
  Wire.endTransmission();
}
boolean checkInterrupt(void){
  //Serial.println(digitalRead(irqpin));
  return digitalRead(irqpin);

}


