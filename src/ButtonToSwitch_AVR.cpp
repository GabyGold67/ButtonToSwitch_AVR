/**
  ******************************************************************************
  * @file	: ButtonToSwitch_AVR.cpp
  * @brief	: Source file for the ButtonToSwitch_AVR library classes
  *
  * @details The library implements classes that model several switch mechanisms
  * replacements out of simple push buttons or similar equivalent digital signal 
  * inputs.
  * By using just a button (a.k.a. momentary switches or momentary push buttons,
  * _**MPB**_ for short from here on) the classes implemented in this library will 
  * manage, calculate and update several parameters to **generate the embedded 
  * behavior of standard electromechanical switches**.
  *
  * @author	: Gabriel D. Goldman
  * @version v4.0.0
  * @date	: Created on: 10/09/2024
  * 		: Last modification: 10/09/2024
  * @copyright GPL-3.0 license
  *
  ******************************************************************************
  * @attention	This library was developed as part of the refactoring process for
  * an industrial machines security enforcement and productivity control
  * (hardware & firmware update). As such every class included complies **AT LEAST**
  * with the provision of the attributes and methods to make the hardware & firmware
  * replacement transparent to the controlled machines. Generic use attribute and
  * methods were added to extend the usability to other projects and application
  * environments, but no fitness nor completeness of those are given but for the
  * intended refactoring project.
  * 
  * @warning **Use of this library is under your own responsibility**
  ******************************************************************************
  */
#include <Arduino.h>
#include <ButtonToSwitch_AVR.h>
#include <TimerOne.h>
//===========================>> BEGIN General use Global variables
	uint8_t DbncdMPBttn::_mpbsCount = 0;
	DbncdMPBttn** _mpbsInstncsLstPtr = nullptr;
	unsigned long int _isrTmrPrd = 0;
//===========================>> END General use Global variables


DbncdMPBttn::DbncdMPBttn()
: _mpbttnPin{_InvalidPinNum}, _pulledUp{true}, _typeNO{true}, _dbncTimeOrigSett{0}
{
}

DbncdMPBttn::DbncdMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett)
: _mpbttnPin{mpbttnPin}, _pulledUp{pulledUp}, _typeNO{typeNO}, _dbncTimeOrigSett{dbncTimeOrigSett}
{

	if(mpbttnPin != _InvalidPinNum){
		String mpbPinNumStr {"00" + String(_mpbttnPin)};
		mpbPinNumStr = mpbPinNumStr.substring(mpbPinNumStr.length() - 2, 2);
		_mpbPollTmrName = "PollMpbPin" + mpbPinNumStr + "_tmr";

		if(_dbncTimeOrigSett < _stdMinDbncTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
			_dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
		_dbncTimeTempSett = _dbncTimeOrigSett;
		pinMode(mpbttnPin, (pulledUp == true)?INPUT_PULLUP:INPUT);
	}
	else{
		_pulledUp = true;
		_typeNO = true;
		_dbncTimeOrigSett = 0;
	}
}

DbncdMPBttn::~DbncdMPBttn(){
    
    end();  // Stops the software timer associated to the object, deletes it's entry and nullyfies the handle to it befor destructing the object
}

bool DbncdMPBttn::begin(const unsigned long int &pollDelayMs) {
    bool result {false};
    BaseType_t tmrModResult {pdFAIL};

    if (pollDelayMs > 0){
        if (!_mpbPollTmrHndl){        
            _mpbPollTmrHndl = xTimerCreate(
                _mpbPollTmrName.c_str(),  //Timer name
                pdMS_TO_TICKS(pollDelayMs),  //Timer period in ticks
                pdTRUE,     //Auto-reload true
                this,       //TimerID: data passed to the callback function to work
                mpbPollCallback  //DbncdMPBttn::mpbPollCallback  //Callback function
            );
        if (_mpbPollTmrHndl != NULL){
            tmrModResult = xTimerStart(_mpbPollTmrHndl, portMAX_DELAY);
            if (tmrModResult == pdPASS)
                result = true;
        }

        }
    }

    return result;
}

void DbncdMPBttn::clrStatus(bool clrIsOn){
    //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    /*To Resume operations after a pause() without risking generating false "Valid presses" and "On" situations,
    several attributes must be resetted to "Start" values.
    The only important value not reseted is the _mpbFdaState, to do it call resetFda() INSTEAD of this method*/
    
    //>e taskENTER_CRITICAL(&mux);   //ESP-IDF FreeRTOS modifies the taskENTER_CRITICAL of vanilla FreeRTOS to require a mutex as argument to avoid core to core interruptions
	_isPressed = false;
	_validPressPend = false;
	_validReleasePend = false;
	_dbncTimerStrt = 0;
	_dbncRlsTimerStrt = 0;
	if(clrIsOn){
		if(_isOn){
			_turnOff();
		}
	}
    //>e taskEXIT_CRITICAL(&mux);
    
    return;
}

void DbncdMPBttn::clrSttChng(){
	_sttChng = false;

	return;
}

void DbncdMPBttn::disable(){

    return _setIsEnabled(false);
}

void DbncdMPBttn::enable(){

    return _setIsEnabled(true);
}

bool DbncdMPBttn::end(){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (_mpbPollTmrHndl){
   	result = pause();
      if (result){
      	tmrModResult = xTimerDelete(_mpbPollTmrHndl, portMAX_DELAY);
			if (tmrModResult == pdPASS){
				_mpbPollTmrHndl = NULL;
			}
			else{
				result = false;
			}
      }
   }

   return result;
}

const unsigned long int DbncdMPBttn::getCurDbncTime() const{

    return _dbncTimeTempSett;
}

fncPtrType DbncdMPBttn::getFnWhnTrnOff(){

	return _fnWhnTrnOff;
}

fncPtrType DbncdMPBttn::getFnWhnTrnOn(){

	return _fnWhnTrnOn;
}

const bool DbncdMPBttn::getIsEnabled() const{

    return _isEnabled;
}

const bool DbncdMPBttn::getIsOn() const {

	return _isOn;
}

const bool DbncdMPBttn::getIsOnDisabled() const{

    return _isOnDisabled;
}

const bool DbncdMPBttn::getIsPressed() const {

    return _isPressed;
}

const uint32_t DbncdMPBttn::getOtptsSttsPkgd(){

	return _otptsSttsPkg();
}

const bool DbncdMPBttn::getOutputsChange() const{

    return _outputsChange;
}

unsigned long int DbncdMPBttn::getStrtDelay(){

	return _strtDelay;
}

/*>e const TaskHandle_t DbncdMPBttn::getTaskToNotify() const{
    
    return _taskToNotifyHndl;
}*/

/*>e const TaskHandle_t DbncdMPBttn::getTaskWhileOn(){

	return _taskWhileOnHndl;
}*/

bool DbncdMPBttn::init(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett){
    bool result {false};

    if((_mpbttnPin == _InvalidPinNum) && (mpbttnPin != _InvalidPinNum)){
        if (_mpbPollTmrName == ""){
            _mpbttnPin = mpbttnPin;
            _pulledUp = pulledUp;
            _typeNO = typeNO;
            _dbncTimeOrigSett = dbncTimeOrigSett;

            String mpbPinNumStr {"00" + String(_mpbttnPin)};
            mpbPinNumStr = mpbPinNumStr.substring(mpbPinNumStr.length() - 2, 2);
            _mpbPollTmrName = "PollMpbPin" + mpbPinNumStr + "_tmr";

            if(_dbncTimeOrigSett < _stdMinDbncTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
                _dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
            _dbncTimeTempSett = _dbncTimeOrigSett;
            //>e pinMode(mpbttnPin, (pulledUp == true)?INPUT_PULLUP:INPUT_PULLDOWN);
            pinMode(mpbttnPin, (pulledUp == true)?INPUT_PULLUP:INPUT);
            result = true;
        }
        else{
            _pulledUp = true;
            _typeNO = true;
            _dbncTimeOrigSett = 0;
        }
    }
    
    return result;
}

void DbncdMPBttn::mpbPollCallback(TimerHandle_t mpbTmrCbArg){
	DbncdMPBttn* mpbObj = (DbncdMPBttn*)pvTimerGetTimerID(mpbTmrCbArg);
	BaseType_t xReturned;
  //>e  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    
  //>e  taskENTER_CRITICAL(&mux);   //ESP-IDF FreeRTOS modifies the taskENTER_CRITICAL of vanilla FreeRTOS to require a mutex as argument to avoid core to core interruptions
	if(mpbObj->getIsEnabled()){
		// Input/Output signals update
		mpbObj->updIsPressed();
		// Flags/Triggers calculation & update
		mpbObj->updValidPressesStatus();
	}
	// State machine status update
	mpbObj->updFdaState();
	//>e taskEXIT_CRITICAL(&mux);

	/*>e if (mpbObj->getOutputsChange()){	//Output changes might happen as part of the updFdaState() execution
		if(mpbObj->getTaskToNotify() != NULL){
			xReturned = xTaskNotify(
					mpbObj->getTaskToNotify(),	//TaskHandle_t of the task receiving notification
					static_cast<unsigned long>(mpbObj->getOtptsSttsPkgd()),
					eSetValueWithOverwrite	//In this specific case using eSetBits is also a valid option
					);
			 if (xReturned != pdPASS){
				 errorFlag = pdTRUE;
			 }
			 mpbObj->setOutputsChange(false);	//If the outputsChange triggers a task to treat it, here's  the flag reset, in other cases the mechanism reading the chganges must take care of the flag status
		}
	}*/

	return;
}

uint32_t DbncdMPBttn::_otptsSttsPkg(uint32_t prevVal){
	if(_isOn){
		prevVal |= ((uint32_t)1) << IsOnBitPos;
	}
	else{
		prevVal &= ~(((uint32_t)1) << IsOnBitPos);
	}
	if(_isEnabled){
		prevVal |= ((uint32_t)1) << IsEnabledBitPos;
	}
	else{
		prevVal &= ~(((uint32_t)1) << IsEnabledBitPos);
	}

	return prevVal;
}

bool DbncdMPBttn::pause(){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   if (_mpbPollTmrHndl){
      if (xTimerIsTimerActive(_mpbPollTmrHndl)){
         tmrModResult = xTimerStop(_mpbPollTmrHndl, portMAX_DELAY);
         if (tmrModResult == pdPASS)
            result = true;
      }
   }
   else{
      result = true;
   }

   return result;
}

void DbncdMPBttn::resetDbncTime(){
   setDbncTime(_dbncTimeOrigSett);

   return; 
}

void DbncdMPBttn::resetFda(){
  //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	clrStatus();
	setSttChng();
	_mpbFdaState = stOffNotVPP;
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

bool DbncdMPBttn::resume(){
   bool result {false};
   BaseType_t tmrModResult {pdFAIL};

   resetFda();	//To restart in a safe situation the FDA is resetted to have all flags and timers cleaned up
	if (_mpbPollTmrHndl){
		if (xTimerIsTimerActive(_mpbPollTmrHndl) == pdFAIL){	// This enforces the timer to be stopped to let the timer be resumed, makes the method useless just to reset the timer counter
			tmrModResult = xTimerReset( _mpbPollTmrHndl, portMAX_DELAY);
			if (tmrModResult == pdPASS)
				result = true;
		}
	}

    return result;
}

bool DbncdMPBttn::setDbncTime(const unsigned long int &newDbncTime){
    bool result {true};
    //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

    //>e taskENTER_CRITICAL(&mux);
    if(_dbncTimeTempSett != newDbncTime){
		 if (newDbncTime >= _stdMinDbncTime){
			  _dbncTimeTempSett = newDbncTime;
		 }
		 else{
			  result = false;
		 }
    }
    //>e taskEXIT_CRITICAL(&mux);

    return result;
}

void DbncdMPBttn::setFnWhnTrnOffPtr(void (*newFnWhnTrnOff)()){
  //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	if (_fnWhnTrnOff != newFnWhnTrnOff){
		_fnWhnTrnOff = newFnWhnTrnOff;
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)()){
  //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	if (_fnWhnTrnOn != newFnWhnTrnOn){
		_fnWhnTrnOn = newFnWhnTrnOn;
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::_setIsEnabled(const bool &newEnabledValue){
   //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	if(_isEnabled != newEnabledValue){
		if (newEnabledValue){  //Change to Enabled = true
			_validEnablePend = true;
			if(_validDisablePend)
				_validDisablePend = false;
		}
		else{	//Change to Enabled = false  (i.e. Disabled)
			_validDisablePend = true;
			if(_validEnablePend)
				_validEnablePend = false;
		}
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::setIsOnDisabled(const bool &newIsOnDisabled){
   //>e portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	if(_isOnDisabled != newIsOnDisabled){
		_isOnDisabled = newIsOnDisabled;
		if(!_isEnabled){
			if(_isOn != _isOnDisabled){
				if(_isOnDisabled){
					_turnOn();
				}
				else{
					_turnOff();
				}
			}
		}
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::setOutputsChange(bool newOutputsChange){
   //>e portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	if(_outputsChange != newOutputsChange)
   	_outputsChange = newOutputsChange;
	//>e taskEXIT_CRITICAL(&mux);

   return;
}

void DbncdMPBttn::setSttChng(){
	_sttChng = true;

	return;
}

/*>e void DbncdMPBttn::setTaskToNotify(const TaskHandle_t &newTaskHandle){
	eTaskState taskWhileOnStts{};
   portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

   taskENTER_CRITICAL(&mux);
   if(_taskToNotifyHndl != newTaskHandle){
      if(_taskToNotifyHndl != NULL){
         taskWhileOnStts = eTaskGetState(_taskToNotifyHndl);
         if (taskWhileOnStts != eSuspended){
            if(taskWhileOnStts != eDeleted){
               vTaskSuspend(_taskToNotifyHndl);
               _taskToNotifyHndl = NULL;
            }
         }
      }
      if (newTaskHandle != NULL){
         _taskToNotifyHndl = newTaskHandle;
      }
   }
   taskEXIT_CRITICAL(&mux);

    return;
}*/

/*>e void DbncdMPBttn::setTaskWhileOn(const TaskHandle_t &newTaskHandle){
	eTaskState taskWhileOnStts{};
   portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

	taskENTER_CRITICAL(&mux);
	if(_taskWhileOnHndl != newTaskHandle){
		if(_taskWhileOnHndl != NULL){
			taskWhileOnStts = eTaskGetState(_taskWhileOnHndl);
			if (taskWhileOnStts != eSuspended){
				if(taskWhileOnStts != eDeleted){
					vTaskSuspend(_taskWhileOnHndl);
					_taskWhileOnHndl = NULL;
				}
			}
		}
		if (newTaskHandle != NULL){
			_taskWhileOnHndl = newTaskHandle;
		}
	}
	taskEXIT_CRITICAL(&mux);

	return;
}*/

void DbncdMPBttn::_turnOff(){
   //>e portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

	if(_isOn){
		//---------------->> Tasks related actions
		/*>e if(_taskWhileOnHndl != NULL){
			eTaskState taskWhileOnStts{eTaskGetState(_taskWhileOnHndl)};
			if (taskWhileOnStts != eSuspended){
				if(taskWhileOnStts != eDeleted){
					vTaskSuspend(_taskWhileOnHndl);
				}
			}
		}*/
		//---------------->> Functions related actions
		if(_fnWhnTrnOff != nullptr){
			_fnWhnTrnOff();
		}
	}

	//>e taskENTER_CRITICAL(&mux);
	if(_isOn){
		//---------------->> Flags related actions
		_isOn = false;
		_outputsChange = true;
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::_turnOn(){
   //>e portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

	if(!_isOn){
		//---------------->> Tasks related actions
		/*>e if(_taskWhileOnHndl != NULL){
			eTaskState taskWhileOnStts{eTaskGetState(_taskWhileOnHndl)};
			if(taskWhileOnStts != eDeleted){
				if (taskWhileOnStts == eSuspended){
					vTaskResume(_taskWhileOnHndl);
				}
			}
		}*/
		//---------------->> Functions related actions
		if(_fnWhnTrnOn != nullptr){
			_fnWhnTrnOn();
		}
	}

	//>e taskENTER_CRITICAL(&mux);
	if(!_isOn){
		//---------------->> Flags related actions
		_isOn = true;
		_outputsChange = true;
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

void DbncdMPBttn::updFdaState(){
   //>e portMUX_TYPE mux portMUX_INITIALIZER_UNLOCKED;

	//>e taskENTER_CRITICAL(&mux);
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn){
				_turnOn();
			}
			_validPressPend = false;
			_mpbFdaState = stOn;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOn:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_isOn){
				_turnOff();
			}
			_validReleasePend = false;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				if(_isOn != _isOnDisabled){
					if(_isOn){
						_turnOff();
					}
					else{
						_turnOn();
					}
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				_isEnabled = false;
				if(!_outputsChange)
					_outputsChange = true;
				_validDisablePend = false;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				if(_isOn){
					_turnOff();
				}
				_isEnabled = true;
				_validEnablePend = false;
				if(!_outputsChange)
					_outputsChange = true;
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
			}	// Execute this code only ONCE, when exiting this state
			break;

	default:
		break;
	}
	//>e taskEXIT_CRITICAL(&mux);

	return;
}

bool DbncdMPBttn::updIsPressed(){
    /*To be 'pressed' the conditions are:
    1) For NO == true
        a)  _pulledUp == false ==> digitalRead == HIGH
        b)  _pulledUp == true ==> digitalRead == LOW
    2) For NO == false
        a)  _pulledUp == false ==> digitalRead == LOW
        b)  _pulledUp == true ==> digitalRead == HIGH
    */
    bool result {false};
    bool tmpPinLvl {digitalRead(_mpbttnPin)};
    
    if (_typeNO == true){
        //For NO MPBs
        if (_pulledUp == false){
            if (tmpPinLvl == HIGH)
                result = true;
        }
        else{
            if (tmpPinLvl == LOW)
                result = true;
        }
    }
    else{
        //For NC MPBs
        if (_pulledUp == false){
            if (tmpPinLvl == LOW)
                result = true;
        }
        else{
            if (tmpPinLvl == HIGH)
                result = true;
        }
    }    
    _isPressed = result;

    return _isPressed;
}

bool DbncdMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(_dbncRlsTimerStrt != 0)
			_dbncRlsTimerStrt = 0;
		if(!_prssRlsCcl){
			if(_dbncTimerStrt == 0){    //This is the first detection of the press event
				//>e _dbncTimerStrt = xTaskGetTickCount() / portTICK_RATE_MS;	//Started to be pressed
				_dbncTimerStrt = millis();	//Started to be pressed
			}
			else{
				//>e if (((xTaskGetTickCount() / portTICK_RATE_MS) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
				if (((millis()) - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
					_validPressPend = true;
					_validReleasePend = false;
					_prssRlsCcl = true;
				}
			}
		}
	}
	else{
		if(_dbncTimerStrt != 0)
			_dbncTimerStrt = 0;
		if(_prssRlsCcl){
			if(_dbncRlsTimerStrt == 0){    //This is the first detection of the release event
				_dbncRlsTimerStrt = millis();	//Started to be UNpressed
			}
			else{
				if (((millis()) - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
					_validReleasePend = true;
					_prssRlsCcl = false;
				}
			}
		}
	}

	return (_validPressPend||_validReleasePend);
}

//=========================================================================> Class methods delimiter

unsigned long SevenSegDisplays::blinkTmrGCD(unsigned long blnkOnTm, unsigned long blnkOffTm){
   /*returning values:
      0: One of the input values was 0
      1: No GDC greater than 1
      Other: This value would make the blink timer save resources by checking the blink time as less frequent as possible*/
   unsigned long result{ 0 };

   if ((blnkOnTm != 0) && (blnkOffTm != 0)) {
      if (blnkOnTm == blnkOffTm) {
         result = blnkOnTm;
      }
      else if ((blnkOnTm % blnkOffTm == 0) || (blnkOffTm % blnkOnTm == 0)) {
         result = (blnkOffTm < blnkOnTm)? blnkOffTm : blnkOnTm;
      }

      if (result == 0) {
         for (unsigned long int i{ (blnkOnTm < blnkOffTm) ? blnkOnTm : blnkOffTm }; i > 0; i--) {
               if ((blnkOnTm % i == 0) && (blnkOffTm % i == 0)) {
                  result = i;
                  break;
               }
         }
      }
   }

   return result;
}

