/**
  ******************************************************************************
  * @file	: 10_HntdTmLtchMPBttn_1a.ino
  * @brief  : Example for the ButtonToSwitch_AVR library HntdTmLtchMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a HntdTmLtchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbWrnngOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbPilotOtpt
  *
  * This simple example instantiates the HntdTmLtchMPBttn object in the setup(),
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
  * @attention	This file is part of the examples folder for the ButtonToSwitch_AVR
  * library. All files needed are provided as part of the source code for the library.
  * 
  * Released into the public domain in accordance with "GPL-3.0-or-later" license terms.
  *
  ******************************************************************************
  */
#include <Arduino.h>
#include <ButtonToSwitch_AVR.h>

const uint8_t dmpbMainInpt{6};
const uint8_t dmpbIsOnOtpt{3};
const uint8_t dmpbWrnngOtpt{5};
const uint8_t dmpbPilotOtpt{7};

HntdTmLtchMPBttn myDMPBttn (dmpbMainInpt, 4000, 25);

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  digitalWrite(dmpbWrnngOtpt, LOW);
  digitalWrite(dmpbPilotOtpt, LOW);

  pinMode(dmpbIsOnOtpt, OUTPUT);
  pinMode(dmpbWrnngOtpt, OUTPUT);
  pinMode(dmpbPilotOtpt, OUTPUT);

  myDMPBttn.setKeepPilot(true);
  myDMPBttn.begin(40);
}

void loop() {
  if(myDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    digitalWrite(dmpbWrnngOtpt, (myDMPBttn.getWrnngOn())?HIGH:LOW);    
    digitalWrite(dmpbPilotOtpt, (myDMPBttn.getPilotOn())?HIGH:LOW);    
    myDMPBttn.setOutputsChange(false);
  }
}  
