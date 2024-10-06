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
  * @version v1.0.0
  * @date	: Created on: 10/09/2024
  * 		: Last modification: 30/09/2024
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
//===========================>> END General use Global variables

//===========================>> BEGIN Base Class Static variables initialization
DbncdMPBttn** DbncdMPBttn::_mpbsInstncsLstPtr = nullptr;	// Pointer to the array of pointers of DbncdMPBttn objects whose state must be kept updated by the Timer
unsigned long int DbncdMPBttn::_updTimerPeriod = 0;	// Time period for the update Timer to be executed. As is only ONE timer for all the DbncdMPBttn objects, the time period must be shared, so a MCD calculation will determine the value to be used for resources use optimization.
	//! BEGIN Test related variables 
	DbncdMPBttn* DbncdMPBttn::test_DmpbPtr = nullptr;
	//! END Test related variables 
//===========================>> END Base Class Static variables initialization

//===========================>> BEGIN Base Class Static methods implementation

void DbncdMPBttn::_ISRMpbsRfrshCb(){
/* The callback function duties:
 * - Verify for a valid **_mpbsInstncsLstPtr**, if it's nullptr something failed, correct it by disabling the timer
 * - Save the **current time** for reference and traverse the list till the end (nullptr).
 * - For each MPB in the list verify the _updTmrAttchd == true
 * - If _updTmrAttchd == true -> verify the ("current time" - _lstPollTime) >= _pollPeriodMs. If the condition is true:
 * 	- Execute the objects mpbPollCallback()
 * 	- Set _lstPollTime = "current time"
*/
	int auxPtr{0};
	unsigned long int curTime{millis()};

	if(_mpbsInstncsLstPtr != nullptr){
		while (*(_mpbsInstncsLstPtr + auxPtr) != nullptr){
			if((*(_mpbsInstncsLstPtr + auxPtr))->getUpdTmrAttchd()){	// The MPB is attached to the update service, check if update time reached
				if((curTime - ((*(_mpbsInstncsLstPtr + auxPtr))->getLstPollTime())) >= ((*(_mpbsInstncsLstPtr + auxPtr))->getPollPeriodMs())){
					(*(_mpbsInstncsLstPtr + auxPtr))->mpbPollCallback();	// Update the MPBttn state
					(*(_mpbsInstncsLstPtr + auxPtr))->_setLstPollTime(curTime);	//Save the timestamp of this last update
				}
			}
			++auxPtr;
		}
		if(auxPtr == 0){//! The _mpbsInstncsLstPtr IS pointing to an empty "list of MPBtns to be updated", the list must be deleted and the _mpbsInstncsLstPtr -> nullptr			
			Timer1.stop();
			Timer1.detachInterrupt();
			delete [] _mpbsInstncsLstPtr;
			_mpbsInstncsLstPtr = nullptr;
		}
	}
	else{
		// There is NO LIST of MPBs to update, but the _ISRMpbsRfrshCb was invoked, so the Timer1 is enabled: Disable Timer1!!
		Timer1.stop();
		Timer1.detachInterrupt();
	}

	return;	
}

//===========================>> END Base Class Static methods implementation

DbncdMPBttn::DbncdMPBttn()
: _mpbttnPin{_InvalidPinNum}, _pulledUp{true}, _typeNO{true}, _dbncTimeOrigSett{0}
{
	_mpbInstnc = this;
}

DbncdMPBttn::DbncdMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett)
: _mpbttnPin{mpbttnPin}, _pulledUp{pulledUp}, _typeNO{typeNO}, _dbncTimeOrigSett{dbncTimeOrigSett}
{

	if(mpbttnPin != _InvalidPinNum){
		if(_dbncTimeOrigSett < _stdMinDbncTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
			_dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
		_dbncTimeTempSett = _dbncTimeOrigSett;
		pinMode(mpbttnPin, (pulledUp == true)?INPUT_PULLUP:INPUT);
		_mpbInstnc = this;

	}
	else{
		_pulledUp = true;
		_typeNO = true;
		_dbncTimeOrigSett = 0;
	}
}

DbncdMPBttn::~DbncdMPBttn(){
    
	end();  // Stops the software timer associated to the object, deletes it's entry and nullyfies the handle to it before destructing the object
}

bool DbncdMPBttn::begin(const unsigned long int &pollDelayMs) {	//! Final coding pending!!!
	bool result {false};
	bool listWasEmpty{false};

	if (pollDelayMs > 0){
		if(_mpbsInstncsLstPtr == nullptr){	// There is no "MPBs to be updated list", so create it			
			_mpbsInstncsLstPtr = new DbncdMPBttn* [1];
			*_mpbsInstncsLstPtr = nullptr;
			listWasEmpty = true;
		}
		_pushMpb(_mpbsInstncsLstPtr, _mpbInstnc);	// Add the MPB to the "MPBs to be updated list"
		_pollPeriodMs = pollDelayMs;	// Set this MPB's PollPeriodMs to the begin() passed argument
		_updTmrAttchd = true;	//Set the MPB object to be updated by the Timer

		// If the Timer is not running, start it!!
		if (listWasEmpty){   // This is the first MPBttn of the list, start timer
			//Initialize the Interrupt timer
			Timer1.attachInterrupt(_ISRMpbsRfrshCb);
			Timer1.initialize(_pollPeriodMs*1000);	// The MPBs manages times in milliseconds, the timer in microseconds
			Timer1.start();			
		}
		else{	// The "MPBs to be updated was not empty, pollTime must be recalculated and if changes set Timer1.setPeriod() invoked
			if(_pollPeriodMs != _updTimerPeriod){
			/*
				//! recalculate _updTimerPeriod
				_updTimerPeriod = _pollPeriodMs; 	//! _updTimerPeriod = calculus of MCD
				Timer1.setPeriod(_updTimerPeriod*1000);
			*/
			}
		}
		result = true;
	}
   return result;
}

bool DbncdMPBttn::getUpdTmrAttchd(){

	return _updTmrAttchd;
}

void DbncdMPBttn::clrStatus(bool clrIsOn){
	/*To Resume operations after a pause() without risking generating false "Valid presses" and "On" situations,
	several attributes must be resetted to "Start" values.
	The only important value not reseted is the _mpbFdaState, to do it call resetFda() INSTEAD of this method*/
	
	_isPressed = false;
	_validPressPend = false;
	_validReleasePend = false;
	_dbncTimerStrt = 0;
	_dbncRlsTimerStrt = 0;
	if(clrIsOn){
		if(_isOn)
			_turnOff();
	}
    
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

	result = pause();
	if (result){
		//! proceed destroying the list space and setting the _mpbsInstncsLstPtr = nullptr
	}
	else{
		result = false;
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

const unsigned long int DbncdMPBttn::getLstPollTime(){
   
	return _lstPollTime;
}

const uint32_t DbncdMPBttn::getOtptsSttsPkgd(){

	return _otptsSttsPkg();
}

const bool DbncdMPBttn::getOutputsChange() const{

	return _outputsChange;
}

const unsigned long int DbncdMPBttn::getPollPeriodMs() {

   return _pollPeriodMs;
}

unsigned long int DbncdMPBttn::getStrtDelay(){

	return _strtDelay;
}

bool DbncdMPBttn::init(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett){
    bool result {false};

	if(_mpbttnPin == _InvalidPinNum){
		_mpbttnPin = mpbttnPin;
		_pulledUp = pulledUp;
		_typeNO = typeNO;
		_dbncTimeOrigSett = dbncTimeOrigSett;

		if(_dbncTimeOrigSett < _stdMinDbncTime) //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
			_dbncTimeOrigSett = _stdMinDbncTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters
		_dbncTimeTempSett = _dbncTimeOrigSett;
		pinMode(mpbttnPin, (pulledUp == true)?INPUT_PULLUP:INPUT);
		result = true;
	}
	else{
		_pulledUp = true;
		_typeNO = true;
		_dbncTimeOrigSett = 0;
	}
	
	return result;
}

void DbncdMPBttn::mpbPollCallback(){
	if(getIsEnabled()){
		// Input/Output signals update
		updIsPressed();
		// Flags/Triggers calculation & update
		updValidPressesStatus();
	}
	// State machine status update
	updFdaState();

	return;
}

void DbncdMPBttn::_popMpb(DbncdMPBttn** &DMpbTmrUpdLst, DbncdMPBttn* mpbToPop){
	int arrSize{0};
	int auxPtr{0};
	bool mpbFnd{false};
	DbncdMPBttn** tmpArrPtr{nullptr};

	while(*(DMpbTmrUpdLst + arrSize) != nullptr){
		if(*(DMpbTmrUpdLst + arrSize) == mpbToPop){
			mpbFnd = true;
		}
		++arrSize;
	}
	if(mpbFnd){
		if(arrSize > 1){
			tmpArrPtr = new DbncdMPBttn* [arrSize];
			arrSize = 0;
			while(*(DMpbTmrUpdLst + arrSize) != nullptr){
				if(*(DMpbTmrUpdLst + arrSize) == mpbToPop){
					++arrSize;
					if(*(DMpbTmrUpdLst + arrSize) == nullptr)
						break;
				}
				*(tmpArrPtr + auxPtr) = *(DMpbTmrUpdLst + arrSize);
				++arrSize;
				++auxPtr;
			}
			*(tmpArrPtr + auxPtr) = nullptr;
			delete [] DMpbTmrUpdLst;
			DMpbTmrUpdLst = tmpArrPtr;
		}
		else{
			Timer1.stop();
			delete [] DMpbTmrUpdLst;
			DMpbTmrUpdLst = nullptr;
		}
	}

	return;
}

void DbncdMPBttn::_pushMpb(DbncdMPBttn** &DMpbTmrUpdLst, DbncdMPBttn* mpbToPush){
	int arrSize{0};
	bool mpbFnd{false};
	DbncdMPBttn** tmpArrPtr{nullptr};

	while(*(DMpbTmrUpdLst + arrSize) != nullptr){
		if(*(DMpbTmrUpdLst + arrSize) == mpbToPush){
			mpbFnd = true;
			break;
		}
		else{
			++arrSize;
		}
	}
	if(!mpbFnd){
		tmpArrPtr = new DbncdMPBttn* [arrSize + 2];
		for (int i{0}; i < arrSize; ++i){
			*(tmpArrPtr + i) = *(DMpbTmrUpdLst + i);
		}
		*(tmpArrPtr + (arrSize + 0)) = mpbToPush;
		*(tmpArrPtr + (arrSize + 1)) = nullptr;
		delete [] DMpbTmrUpdLst;
		DMpbTmrUpdLst = tmpArrPtr;
	}

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

   // Check if the MPB is in the  _mpbsInstncsLstPtr
	//If it's in the list verify the Timer1 status 
		// If it's not paused, Pause it
		// POP the MPB from the _mpbsInstncsLstPtr list
		// Verify there are MPBs remaining in the list
			// If there are MPBs 
				//Recalculate the _updTimerPeriod
				//set the new _updTimerPeriod to Timer1
				// Resume Timer1
			// If there are not MPBs END the Timer1
	

   return result;
}

void DbncdMPBttn::resetDbncTime(){
   setDbncTime(_dbncTimeOrigSett);

   return; 
}

void DbncdMPBttn::resetFda(){
	clrStatus();
	setSttChng();
	_mpbFdaState = stOffNotVPP;

	return;
}

bool DbncdMPBttn::resume(){
   bool result {false};


	return result;
}

bool DbncdMPBttn::setDbncTime(const unsigned long int &newDbncTime){
	bool result {true};

	if(_dbncTimeTempSett != newDbncTime){
		if (newDbncTime >= _stdMinDbncTime){
			_dbncTimeTempSett = newDbncTime;
		}
		else{
			result = false;
		}
	}

	return result;
}

void DbncdMPBttn::setFnWhnTrnOffPtr(void (*newFnWhnTrnOff)()){

	if (_fnWhnTrnOff != newFnWhnTrnOff){
		_fnWhnTrnOff = newFnWhnTrnOff;
	}

	return;
}

void DbncdMPBttn::setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)()){
	if (_fnWhnTrnOn != newFnWhnTrnOn){
		_fnWhnTrnOn = newFnWhnTrnOn;
	}

	return;
}

void DbncdMPBttn::_setIsEnabled(const bool &newEnabledValue){
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

	return;
}

void DbncdMPBttn::setIsOnDisabled(const bool &newIsOnDisabled){
	if(_isOnDisabled != newIsOnDisabled){
		_isOnDisabled = newIsOnDisabled;
		if(!_isEnabled){
			if(_isOn != _isOnDisabled){
				if(_isOnDisabled)
					_turnOn();
				else
					_turnOff();
			}
		}
	}

	return;
}

void DbncdMPBttn::_setLstPollTime(const unsigned long int &newLstPollTIme){
	if (_lstPollTime != newLstPollTIme)
		_lstPollTime = newLstPollTIme;

	return;
}

void DbncdMPBttn::setOutputsChange(bool newOutputsChange){
	if(_outputsChange != newOutputsChange)
   	_outputsChange = newOutputsChange;

   return;
}

void DbncdMPBttn::setSttChng(){
	_sttChng = true;

	return;
}

void DbncdMPBttn::_turnOff(){

	if(_isOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOff != nullptr){
			_fnWhnTrnOff();
		}
	// }
	// if(_isOn){
		//---------------->> Flags related actions
		_isOn = false;
		_outputsChange = true;
	}

	return;
}

void DbncdMPBttn::_turnOn(){

	if(!_isOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOn != nullptr){
			_fnWhnTrnOn();
		}
	// }
	// if(!_isOn){
		//---------------->> Flags related actions
		_isOn = true;
		_outputsChange = true;
	}

	return;
}

void DbncdMPBttn::updFdaState(){
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
			if(!_isOn)
				_turnOn();
			_validPressPend = false;
			_mpbFdaState = stOn;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//			break;

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
			if(_isOn)
				_turnOff();
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
					if(_isOn)
						_turnOff();
					else
						_turnOn();
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
				if(_isOn)
					_turnOff();
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
			if(_sttChng){	// Execute this code only ONCE, when exiting this state
//				clrStatus(true);	//Uneeded as is the first function executed in the next state (stOffNotVPP)
			}
			break;

	default:
		break;
	}

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

unsigned long int DbncdMPBttn::_updTmrsMCDCalc(DbncdMPBttn** mpbsLstPtr){
   /*returning values:
      0: One of the input values was 0, or the MPBs list is empty: invalid result
      Other: This value would make the MPBs update timer save resources */
   unsigned long int MCD{0};
	int MPBttnsQty {0};
	int MPBttnsActv{0};
	int auxPtr{0};

	if(mpbsLstPtr != nullptr){
		// The list of MPBs to be updated is not empty, there's at least one MPB
		while (*(_mpbsInstncsLstPtr + auxPtr) != nullptr){
			if ((*_mpbsInstncsLstPtr + auxPtr)->getUpdTmrAttchd()){
				if(MCD == 0){
					MCD = (*_mpbsInstncsLstPtr + auxPtr)->getPollPeriodMs();
				}
				else{
					MCD = findMCD(MCD, (*_mpbsInstncsLstPtr + auxPtr)->getPollPeriodMs());
				}
			}
		}
		++auxPtr;
	}

   return MCD;
}

bool DbncdMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(_dbncRlsTimerStrt != 0)
			_dbncRlsTimerStrt = 0;
		if(!_prssRlsCcl){
			if(_dbncTimerStrt == 0){    //This is the first detection of the press event
				_dbncTimerStrt = millis();	//Started to be pressed
			}
			else{
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

unsigned long int findMCD(unsigned long int a, unsigned long int b) {
    unsigned long int result{ 0 };

    // Handle edge cases
    if (a != 0 && b != 0) {
        // Use Euclidean algorithm for efficiency
        while (a != b) {
            if (a > b) {
                a -= b;
            }
            else {
                b -= a;
            }
        }
        result = a;
    }

    return result; // At this point, a and b are equal and represent the MCD
}
