/**
  ******************************************************************************
  * @file	: 13_SnglSrvcVdblMPBttn_1a.ino
  * @brief  : Example for the ButtonToSwitch library SnglSrvcVdblMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a SnglSrvcVdblMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbVoidedOtpt
  *
  * This simple example instantiates the SnglSrvcVdblMPBttn object in the setup(),
  * and then checks it's attributes flags through the getters methods in the
  * loop().
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  * 
  * WOKWI simulation available at:
  * https://wokwi.com/projects/414290746524189697
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
const uint8_t dmpbVoidedOtpt{8};

SnglSrvcVdblMPBttn myDMPBttn (dmpbMainInpt);

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  digitalWrite(dmpbVoidedOtpt, LOW);
  pinMode(dmpbIsOnOtpt, OUTPUT);
  pinMode(dmpbVoidedOtpt, OUTPUT);

  myDMPBttn.setStrtDelay(100);
  myDMPBttn.begin(20);
}

void loop() {
  if(myDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    digitalWrite(dmpbVoidedOtpt, (myDMPBttn.getIsVoided())?HIGH:LOW);    
    myDMPBttn.setOutputsChange(false);
  }
}  
