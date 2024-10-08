/**
  ******************************************************************************
  * @file	: 01_DbncdMPBttn_1a.ino
  * @brief  : Example for the ButtonToSwitch_AVR library DbncdMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a DbncdMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  *
  * This simple example instantiates the DbncdMPBttn object in the setup(),
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
// const uint8_t dmpbIsEnabledOtpt{4};
// const uint8_t dmpbWrnngOtpt{5};
// const uint8_t dmpbPilotOtpt{7};
// const uint8_t dmpbVoidedOtpt{8};
const uint8_t dmpbIsOnScndryOtpt{9};

DbncdMPBttn myDMPBttn (dmpbMainInpt);
DbncdDlydMPBttn myScndryDMPBttn (dmpbAuxInpt);

void setup() {

  //! Testing purposes, delete in production
  Serial.begin(9600);
  delay(100);
  Serial.print("Serial Monitor started\n");
  //! End testing purposes

  digitalWrite(dmpbIsOnOtpt, LOW);
  // digitalWrite(dmpbIsEnabledOtpt, LOW);
  // digitalWrite(dmpbWrnngOtpt, LOW);
  // digitalWrite(dmpbPilotOtpt, LOW);
  // digitalWrite(dmpbVoidedOtpt, LOW);
  digitalWrite(dmpbIsOnScndryOtpt, LOW);
//----
  pinMode(dmpbIsOnOtpt, OUTPUT);
  // pinMode(dmpbIsEnabledOtpt, OUTPUT);
  // pinMode(dmpbWrnngOtpt, OUTPUT);
  // pinMode(dmpbPilotOtpt, OUTPUT);
  // pinMode(dmpbVoidedOtpt, OUTPUT);
  pinMode(dmpbIsOnScndryOtpt, OUTPUT);

  myDMPBttn.begin(40);
  
  myScndryDMPBttn.setStrtDelay(450);
  myScndryDMPBttn.begin(20);
  
}

void loop() {
  if(myDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    myDMPBttn.setOutputsChange(false);
  }

  if(myScndryDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnScndryOtpt, (myScndryDMPBttn.getIsOn())?HIGH:LOW);    
    myScndryDMPBttn.setOutputsChange(false);
  }

}  
