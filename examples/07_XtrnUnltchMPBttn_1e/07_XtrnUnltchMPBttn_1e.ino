/**
  ******************************************************************************
  * @file	: 07_XtrnUnltchMPBttn_1e.ino
  * @brief  : Example for the ButtonToSwitch library XtrnUnltchMPBttn class
  *
  *   Framework: Arduino
  *   Platform: AVR
  * 
  * The example instantiates a XtrnUnltchMPBttn object using:
  * 	- 1 push button between GND and dmpbMainInpt
  * 	- 1 push button between GND and dmpbAuxInpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsOnOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbIsEnabledOtpt
  * 	- 1 led with it's corresponding resistor between GND and dmpbOnOffFnOtpt
  *
  * This simple example instantiates the XtrnUnltchMPBttn object in the setup(),
  * and then checks it's attributes flags through the getters methods in the
  * loop(). Two functions are added to be executed when the object's isOn 
  * attribute flag value changes.
  * 
  * The fnWhnTrnOn() will be called when the object enters the isOn=True state, 
  * and fnWhnTrnOff() when the object enters the isOn=false state.
  * For this example purpose the functions code was kept to the bare minimum,
  * please keep in mind that for more complex and time consuming functions a valid
  * approach would be to treat the function as an INT callback execution: instead
  * of executing a complex and time consuming code in the function set flags and 
  * values needed to identify the situation in the loop() and add the needed code
  * at it.
  * 
  * When a change in the object's outputs attribute flags values is detected, it
  * manages the loads and resources that the switch turns On and Off, in this
  * example case are the output of some GPIO pins.
  * 
  * A time controlling code section changes the isEnabled attribute of the MPBttn
  * after a time period, changing the MPBttn from enabled to disabled and then
  * back, to test the MPBttn behavior when enabling and disabling the MPBttn.
  * 
  * Note: The setIsOnDisabled() method affects the behavior of the MPBttn when it
  * enters the Disabled state, check the documentation and experiment with it.
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

//==========================================>> BEGIN Function Prototypes
void fnWhnTrnOn();
void fnWhnTrnOff();
//==========================================>> END Function Prototypes

//------------------------------> Inputs/Outputs related values
const uint8_t dmpbMainInpt{6};
const uint8_t dmpbAuxInpt{2};
const uint8_t dmpbIsOnOtpt{3};
const uint8_t dmpbIsEnabledOtpt{4};
const uint8_t dmpbOnOffFnOtpt{10};

//------------------------------> isEnabled state related values
unsigned long int enbldOnOffTm{10000};
unsigned long int lstEnblSwpTm{0};

//------------------------------> Function related variables
unsigned long int blinkDuration{0};
unsigned long int blinkLastSwapTime{0};
uint8_t blinksPending{0};
bool blinkStateOn{false};
bool onOffFnPend{false};

//------------------------------> XtrnUnltchMPBttn class object instantiation
DbncdDlydMPBttn myUnltchMPBttn (dmpbAuxInpt);
DbncdDlydMPBttn* myUnltchMPBttnPtr{&myUnltchMPBttn};

XtrnUnltchMPBttn myDMPBttn(dmpbMainInpt, myUnltchMPBttnPtr, true, true, 50, 50);

void setup() {
   digitalWrite(dmpbIsOnOtpt, LOW);
   digitalWrite(dmpbIsEnabledOtpt, LOW);
   digitalWrite(dmpbOnOffFnOtpt, LOW);

   pinMode(dmpbIsOnOtpt, OUTPUT);
   pinMode(dmpbIsEnabledOtpt, OUTPUT);
   pinMode(dmpbOnOffFnOtpt, OUTPUT);

   myDMPBttn.setFnWhnTrnOnPtr(fnWhnTrnOn);
   myDMPBttn.setFnWhnTrnOffPtr(fnWhnTrnOff);
   myDMPBttn.setStrtDelay(50);
   myDMPBttn.setIsOnDisabled(false);
   
   myDMPBttn.begin(40); 
}

void loop() {
   if((millis() - lstEnblSwpTm) > enbldOnOffTm ){
      if(myDMPBttn.getIsEnabled() == true)
         myDMPBttn.disable();
      else
         myDMPBttn.enable();
      lstEnblSwpTm = millis();
   }

   if(myDMPBttn.getOutputsChange()){ //This checking is done for saving resources, avoiding the rewriting of the pin value if there are no state changes in the MPB status
      digitalWrite(dmpbIsOnOtpt, (myDMPBttn.getIsOn())?HIGH:LOW);    
      digitalWrite(dmpbIsEnabledOtpt, (myDMPBttn.getIsEnabled())?LOW:HIGH);    
      myDMPBttn.setOutputsChange(false); //If the OutputChanges attibute flag is used, reset it's value to detect the next need to refresh outputs.
   }

   if(onOffFnPend){
      if((millis() - blinkLastSwapTime) >= blinkDuration){
         blinkStateOn = !blinkStateOn;
         digitalWrite(dmpbOnOffFnOtpt, (blinkStateOn)?HIGH:LOW); 
         blinkLastSwapTime = millis();
         if(!blinkStateOn)
            --blinksPending;
         if(blinksPending == 0)
            onOffFnPend = false;
      }
   }
}  

//==========================================>> BEGIN Functions declarations to executed when isOn status is modified
void fnWhnTrnOn(){
   blinkDuration = 100;
   blinksPending = 1;
   blinkStateOn = false;
   blinkLastSwapTime = 0;
   onOffFnPend = true;

   return;
}

void fnWhnTrnOff(){
   blinkDuration = 200;
   blinksPending = 2;
   blinkStateOn = false;
   blinkLastSwapTime = 0;
   onOffFnPend = true;
  
   return;
}
//==========================================>> END Functions declarations to executed when isOn status is modified
