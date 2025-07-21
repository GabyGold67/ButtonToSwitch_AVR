/**
  ******************************************************************************
  * @file	: 11_SldrDALtchMPBttn_1f.ino
  * @brief  : Example for the ButtonToSwitch library SldrDALtchMPBttn class
  *
  * Repository: https://github.com/GabyGold67/ButtonToSwitch_AVR
  * WOKWI simulation URL: https://wokwi.com/projects/437012667242467329
  * 
  * Framework: Arduino
  * Platform: *
  * 
  * @details The example instantiates a SldrDALtchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbScndModeOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbSldUpOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsMinOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsMaxOtpt
  * 
  * This simple example instantiates the SldrDALtchMPBttn object in the setup(),
  * and checks it's attributes flags through the getters methods.
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  * 
  * @author	: Gabriel D. Goldman
  * mail <gdgoldman67@hotmail.com>
  * Github <https://github.com/GabyGold67>
  *
  * @date First release: 01/08/2023 
  *       Last update:   20/07/2025 18:50 GMT+0200 DST
  ******************************************************************************
  * @warning **Use of this library is under your own responsibility**
  * 
  * @warning The use of this library falls in the category described by The Alan 
  * Parsons Project (c) 1980 "Games People play" disclaimer:  
  * Games people play, you take it or you leave it  
  * Things that they say aren't alright  
  * If I promised you the moon and the stars, would you believe it?  
  * 
  * Released into the public domain in accordance with "GPL-3.0-or-later" license terms.
  ******************************************************************************
*/
#include <Arduino.h>
#include <ButtonToSwitch.h>

//==========================================>> BEGIN User Function Prototypes
void fnWhnScndModeTrnOn();
void fnWhnScndModeTrnOff();

void fnWhnDirUpTrnOn();
void fnWhnDirUpTrnOff();

void fnWhnMinTrnOn();
void fnWhnMinTrnOff();

void fnWhnMaxTrnOn();
void fnWhnMaxTrnOff();
//==========================================>> END User Function Prototypes

const uint8_t dmpbMainInpt{A0};
const uint8_t dmpbIsOnOtpt{3};

const uint8_t dmpbScndModeOtpt{4};
const uint8_t dmpbSldUpOtpt{8};
const uint8_t dmpbIsMinOtpt{10};
const uint8_t dmpbIsMaxOtpt{11};

SldrDALtchMPBttn myDMPBttn(dmpbMainInpt, true, true, 50, 100, 1280);

void setup() {

Serial.begin(9600);
Serial.println("Service strarted");
Serial.println("================");

   digitalWrite(dmpbIsOnOtpt, LOW);
   pinMode(dmpbIsOnOtpt, OUTPUT);
   
   digitalWrite(dmpbScndModeOtpt, LOW);
   pinMode(dmpbScndModeOtpt, OUTPUT);

   digitalWrite(dmpbSldUpOtpt, LOW);
   pinMode(dmpbSldUpOtpt, OUTPUT);

   digitalWrite(dmpbIsMinOtpt, LOW);
   pinMode(dmpbIsMinOtpt, OUTPUT);

   digitalWrite(dmpbIsMaxOtpt, LOW);
   pinMode(dmpbIsMaxOtpt, OUTPUT);

   myDMPBttn.setOtptValMin(255);   // Set minimum value to 10% of the total range
   myDMPBttn.setOtptValMax(2550);  // Set the maximum value to 100% of the total range
   myDMPBttn.setSwpDirOnEnd(false);   // This sets the SldrDALtchMPBttn dimmer NOT to change the "dimming direction" when reaching the set minimum and maximum values
   myDMPBttn.setSwpDirOnPrss(true);   // This sets the SldrDALtchMPBttn dimmer to change the "dimming direction" every time the MPB is pressed to enter the Secondary behavior
   myDMPBttn.setSldrDirUp(); // This sets the dimming direction to start incrementing its value, BUT as the setSwpDirOnPrss() indicates it must change direction as it is pressed, it will start changing directions to Down, and then start the changing values process, si the first time it will start dimming off the led brightness
   myDMPBttn.setOtptSldrStpSize(1);
   myDMPBttn.setScndModActvDly(2000);

   // BEGIN Function for events mechanism assignation
   myDMPBttn.setFnWhnTrnOnScndryPtr(fnWhnScndModeTrnOn);
   myDMPBttn.setFnWhnTrnOffScndryPtr(fnWhnScndModeTrnOff);

   myDMPBttn.setFnWhnTrnOffSldrDirUp(fnWhnDirUpTrnOff);
   myDMPBttn.setFnWhnTrnOnSldrDirUp(fnWhnDirUpTrnOn);
   
   myDMPBttn.setFnWhnTrnOnSldrMinPtr(fnWhnMinTrnOn);
   myDMPBttn.setFnWhnTrnOffSldrMinPtr(fnWhnMinTrnOff);

   myDMPBttn.setFnWhnTrnOnSldrMaxPtr(fnWhnMaxTrnOn);
   myDMPBttn.setFnWhnTrnOffSldrMaxPtr(fnWhnMaxTrnOff);
   // END Function for events mechanism assignation

   myDMPBttn.begin(5);  
Serial.print("Current output value: ");
Serial.println(myDMPBttn.getOtptCurVal());
}

void loop() {
   if(myDMPBttn.getOutputsChange()){
Serial.print("Current output value: ");
Serial.println(myDMPBttn.getOtptCurVal());

      analogWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?(myDMPBttn.getOtptCurVal()/10):0);

      myDMPBttn.setOutputsChange(false);
   }
}  

//==========================================>> BEGIN User Function Definition
void fnWhnScndModeTrnOn(){
   digitalWrite(dmpbScndModeOtpt, HIGH);

   return;
}

void fnWhnScndModeTrnOff(){
   digitalWrite(dmpbScndModeOtpt, LOW);

   return;
}

void fnWhnDirUpTrnOn(){
   digitalWrite(dmpbSldUpOtpt, HIGH);

   return;
}

void fnWhnDirUpTrnOff(){
   digitalWrite(dmpbSldUpOtpt, LOW);

   return;
}

void fnWhnMinTrnOn(){
   digitalWrite(dmpbIsMinOtpt, HIGH);

   return;
}

void fnWhnMinTrnOff(){
   digitalWrite(dmpbIsMinOtpt, LOW);

   return;
}

void fnWhnMaxTrnOn(){
   digitalWrite(dmpbIsMaxOtpt, HIGH);

   return;
}

void fnWhnMaxTrnOff(){
   digitalWrite(dmpbIsMaxOtpt, LOW);

   return;
}

//==========================================>> END User Function Definition
