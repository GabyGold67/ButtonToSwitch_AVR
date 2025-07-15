/**
  ******************************************************************************
  * @file	: 07_XtrnUnltchMPBttn_2a.ino
  * @brief  : Example for the ButtonToSwitch library XtrnUnltchMPBttn class
  *
  * Repository: https://github.com/GabyGold67/ButtonToSwitch_AVR
  * WOKWI simulation URL: https://wokwi.com/projects/414261841829819393
  * 
  * Framework: Arduino
  * Platform: *
  * 
  * @details The example instantiates a XtrnUnltchMPBttn object using:
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
  * @author	: Gabriel D. Goldman
  * mail <gdgoldman67@hotmail.com>
  * Github <https://github.com/GabyGold67>
  *
  * @date First release: 01/08/2023 
  *       Last update:   07/10/2024 14:20 GMT+0200 DST
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

const uint8_t dmpbMainInpt{A0};
const uint8_t dmpbAuxInpt{A1};

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
