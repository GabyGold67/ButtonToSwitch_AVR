/**
  ******************************************************************************
  * @file	: 11_SldrDALtchMPBttn_1a.ino
  * @brief  : Example for the ButtonToSwitch library SldrDALtchMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a SldrDALtchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  *
  * This simple example instantiates the SldrDALtchMPBttn object in the setup(),
  * and checks it's attributes flags through the getters methods.
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  *
  * 	@author	: Gabriel D. Goldman
  *
  * 	@date	: 	01/08/2023 First release
  * 				    07/10/2024 Last update
  *
  ******************************************************************************
  * @attention	This file is part of the examples folder for the ButtonToSwitch
  * library. All files needed are provided as part of the source code for the library.
  * 
  * Released into the public domain in accordance with "GPL-3.0-or-later" license terms.
  *
  ******************************************************************************
  */
#include <Arduino.h>
#include <ButtonToSwitch.h>

const uint8_t dmpbMainInpt{6};
const uint8_t dmpbIsOnOtpt{3};

SldrDALtchMPBttn myDMPBttn(dmpbMainInpt, true, true, 50, 100, 1280);

void setup() {

   digitalWrite(dmpbIsOnOtpt, LOW);
   pinMode(dmpbIsOnOtpt, OUTPUT);

   myDMPBttn.setOtptValMin(255);   // Set minimum value to 10% of the total range
   myDMPBttn.setOtptValMax(2550);  // Set the maximum value to 100% of the total range
   myDMPBttn.setSwpDirOnEnd(false);   // This sets the SldrDALtchMPBttn dimmer NOT to change the "dimming direction" when reaching the set minimum and maximum values
   myDMPBttn.setSwpDirOnPrss(true);   // This sets the SldrDALtchMPBttn dimmer to change the "dimming direction" every time the MPB is pressed to enter the Secondary behavior
   myDMPBttn.setSldrDirUp(); // This sets the dimming direction to start incrementing its value, BUT as the setSwpDirOnPrss() indicates it must change direction as it is pressed, it will start changing directions to Down, and then start the changing values process, si the first time it will start dimming off the led brightness
   myDMPBttn.setOtptSldrStpSize(1);
   myDMPBttn.setScndModActvDly(2000);
   myDMPBttn.begin(5);  
}

void loop() {
   if(myDMPBttn.getOutputsChange()){
      analogWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?(myDMPBttn.getOtptCurVal()/10):0);

      myDMPBttn.setOutputsChange(false);
   }
}  
