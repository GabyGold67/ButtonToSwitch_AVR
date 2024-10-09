/**
  ******************************************************************************
  * @file	: 07_XtrnUnltchMPBttn_2a.ino
  * @brief  : Example for the ButtonToSwitch_AVR library XtrnUnltchMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a XtrnUnltchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 push button between GND and dmpbAuxInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  *
  * This simple example instantiates the XtrnUnltchMPBttn object in the setup(),
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
const uint8_t dmpbAuxInpt{2};

const uint8_t dmpbIsOnOtpt{3};

DbncdDlydMPBttn myUnltchMPBttn(dmpbAuxInpt);
XtrnUnltchMPBttn myDMPBttn(dmpbMainInpt, true, true, 50, 50);

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  pinMode(dmpbIsOnOtpt, OUTPUT);

  myDMPBttn.begin(40);
  myUnltchMPBttn.begin(20);  
}

void loop() {
  if(myUnltchMPBttn.getOutputsChange()){ //This checking is done for saving resources, avoiding the rewriting of the pin value if there are no state changes in the MPB status
    if(myUnltchMPBttn.getIsOn())
      myDMPBttn.unlatch();
    myUnltchMPBttn.setOutputsChange(false); //If the OutputChanges attibute flag is used, reset it's value to detect the next need to refresh outputs.
  }

  if(myDMPBttn.getOutputsChange()){ //This checking is done for saving resources, avoiding the rewriting of the pin value if there are no state changes in the MPB status
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    myDMPBttn.setOutputsChange(false); //If the OutputChanges attibute flag is used, reset it's value to detect the next need to refresh outputs.
  }
}  
