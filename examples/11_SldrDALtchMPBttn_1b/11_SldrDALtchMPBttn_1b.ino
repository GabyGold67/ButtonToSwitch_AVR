/**
  ******************************************************************************
  * @file	: 11_SldrDALtchMPBttn_1b.ino
  * @brief  : Example for the ButtonToSwitch library SldrDALtchMPBttn class
  *
  * Repository: https://github.com/GabyGold67/ButtonToSwitch_AVR
  * WOKWI simulation URL: https://wokwi.com/projects/414362021424667649
  * 
  * Framework: Arduino
  * Platform: *
  * 
  * @details The example instantiates a SldrDALtchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsEnabledOtpt
  *
  * This simple example instantiates the SldrDALtchMPBttn object in the setup(),
  * and checks it's attributes flags through the getters methods.
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  * 
  * A time controlling code section changes the isEnabled attribute of the MPBttn
  * after a time period, changing the MPBttn from enabled to disabled and then
  * back, to test the MPBttn behavior when enabling and disabling the MPBttn
  * 
  * Note: The setIsOnDisabled() method affects the behavior of the MPBttn, check
  * the documentation and experiment with it.
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
const uint8_t dmpbIsEnabledOtpt{4};

SldrDALtchMPBttn myDMPBttn(dmpbMainInpt, true, true, 50, 100, 1280);

unsigned long int enbldOnOffTm{10000};
unsigned long int lstSwpTm{0};

void setup() {
  digitalWrite(dmpbIsOnOtpt, LOW);
  digitalWrite(dmpbIsEnabledOtpt, LOW);

  pinMode(dmpbIsOnOtpt, OUTPUT);
  pinMode(dmpbIsEnabledOtpt, OUTPUT);

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
  if((millis() - lstSwpTm) > enbldOnOffTm ){
    if(myDMPBttn.getIsEnabled() == true)
      myDMPBttn.disable();
    else
      myDMPBttn.enable();
    lstSwpTm = millis();
  }

   if(myDMPBttn.getOutputsChange()){ //This checking is done for saving resources, avoiding the rewriting of the pin value if there are no state changes in the MPB status
      analogWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?(myDMPBttn.getOtptCurVal()/10):0);
      digitalWrite(dmpbIsEnabledOtpt, (myDMPBttn.getIsEnabled())?LOW:HIGH);    
      myDMPBttn.setOutputsChange(false); //If the OutputChanges attibute flag is used, reset it's value to detect the next need to refresh outputs.
   }
}  
