/**
  ******************************************************************************
  * @file	: 12_DDlydDALtchMPBttn_1a.ino
  * @brief  : Example for the ButtonToSwitch library DDlydDALtchMPBttn class
  *
  * Repository: https://github.com/GabyGold67/ButtonToSwitch_AVR
  * WOKWI simulation URL: https://wokwi.com/projects/414290468155549697
  * 
  * Framework: Arduino
  * Platform: *
  * 
  * @details The example instantiates a DDlydDALtchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnScndryOtpt
  *
  * This simple example instantiates the DDlydDALtchMPBttn object in the setup(),
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

const uint8_t dmpbIsOnOtpt{3};
const uint8_t dmpbIsOnScndryOtpt{9};

DDlydDALtchMPBttn myDMPBttn (dmpbMainInpt);

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  digitalWrite(dmpbIsOnScndryOtpt, LOW);

  pinMode(dmpbIsOnOtpt, OUTPUT);
  pinMode(dmpbIsOnScndryOtpt, OUTPUT);

  myDMPBttn.begin(40);  
}

void loop() {
  if(myDMPBttn.getOutputsChange()){
    digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
    digitalWrite(dmpbIsOnScndryOtpt, (myDMPBttn.getIsOnScndry())?HIGH:LOW);    
    myDMPBttn.setOutputsChange(false);
  }
}  
