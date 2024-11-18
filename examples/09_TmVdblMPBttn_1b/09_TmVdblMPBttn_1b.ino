/**
  ******************************************************************************
  * @file	: 09_TmVdblMPBttn_1b.ino
  * @brief  : Example for the ButtonToSwitch library TmVdblMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a TmVdblMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbVoidedOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsEnabledOtpt
  *
  * This simple example instantiates the TmVdblMPBttn object in the setup(),
  * and then checks it's attributes flags through the getters methods in the
  * loop().
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  *
  * A time controlling code section changes the isEnabled attribute of the MPBttn
  * after a time period, changing the MPBttn from enabled to disabled and then
  * back, to test the MPBttn behavior when enabling and disabling the MPBttn
  * 
  * Note: The setIsOnDisabled() method affects the behavior of the MPBttn when it
  * enters the Disabled state, check the documentation and experiment with it.
  * 
  * WOKWI simulation available at:
  * https://wokwi.com/projects/414349155075989505
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
const uint8_t dmpbIsEnabledOtpt{4};

TmVdblMPBttn myDMPBttn (dmpbMainInpt, 3000);

unsigned long int enbldOnOffTm{10000};
unsigned long int lstSwpTm{0};

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  digitalWrite(dmpbVoidedOtpt, LOW);
  digitalWrite(dmpbIsEnabledOtpt, LOW);

  pinMode(dmpbIsOnOtpt, OUTPUT);
  pinMode(dmpbVoidedOtpt, OUTPUT);
  pinMode(dmpbIsEnabledOtpt, OUTPUT);

  myDMPBttn.setIsOnDisabled(false);
  myDMPBttn.setStrtDelay(100);
  myDMPBttn.begin(20);
}

void loop() {
  if((millis() - lstSwpTm) > enbldOnOffTm ){
    if(myDMPBttn.getIsEnabled() == true)
      myDMPBttn.disable();
    else
      myDMPBttn.enable();
    lstSwpTm = millis();
  }

  if(myDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    digitalWrite(dmpbVoidedOtpt, (myDMPBttn.getIsVoided())?HIGH:LOW);    
    digitalWrite(dmpbIsEnabledOtpt, (myDMPBttn.getIsEnabled())?LOW:HIGH);    
    myDMPBttn.setOutputsChange(false);
  }
}  
