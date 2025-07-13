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
  * Repository: https://github.com/GabyGold67/ButtonToSwitch_AVR
  * 
  * Framework: Arduino  
  * Platform: *
  * 
  * @author Gabriel D. Goldman  
  * mail <gdgoldman67@hotmail.com>  
  * Github <https://github.com/GabyGold67>  
  * 
  * @version v4.6.0
  * 
  * @date First release: 10/09/2024  
  *       Last update:   13/07/2025 12:10 (GMT+0200) DST  
  * 
  * @copyright Copyright (c) 2025  GPL-3.0 license  
  *******************************************************************************
  * @attention	This library was originally developed as part of the refactoring
  * process for an industrial machines security enforcement and productivity control
  * (hardware & firmware update). As such every class included complies **AT LEAST**
  * with the provision of the attributes and methods to make the hardware & firmware
  * replacement transparent to the controlled machines. Generic use attributes and
  * methods were added to extend the usability to other projects and application
  * environments, but no fitness nor completeness of those are given but for the
  * intended refactoring project, and for the author's projects requirements.  
  * 
  * @warning **Use of this library is under your own responsibility**
  * 
  * @warning The use of this library falls in the category described by The Alan 
  * Parsons Project (c) 1980 Games People play disclaimer:   
  * Games people play, you take it or you leave it  
  * Things that they say aren't alright  
  * If I promised you the moon and the stars, would you believe it?  
 *******************************************************************************
 */
#include <Arduino.h>
#include <ButtonToSwitch.h>
#include <TimerOne.h>
//===========================>> BEGIN General use Global variables
//===========================>> END General use Global variables

//===========================>> BEGIN Base Class Static variables initialization
DbncdMPBttn** DbncdMPBttn::_mpbsInstncsLstPtr = nullptr;	// Pointer to the array of pointers of DbncdMPBttn objects whose state must be kept updated by the Timer
unsigned long int DbncdMPBttn::_updTimerPeriod = 0;	// Time period for the update Timer to be executed. As is only ONE timer for all the DbncdMPBttn objects, the time period must be shared, so a MCD calculation will determine the value to be used for resources use optimization. The non-valid 0 value will be used as a flag to signal the service is not active, activation must be done after setting the new operations value.
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
		_mpbInstnc = this;

	}
	else{
		_pulledUp = true;
		_typeNO = true;
		_dbncTimeOrigSett = 0;
	}
}

DbncdMPBttn::DbncdMPBttn(const DbncdMPBttn &other)
: _mpbttnPin{other._mpbttnPin}, _pulledUp{other._pulledUp},	_typeNO{other._typeNO},	_dbncTimeOrigSett{other._dbncTimeOrigSett}
{
	_mpbInstnc = this;
	_dbncTimeTempSett = other._dbncTimeTempSett;
	_pollPeriodMs = other._pollPeriodMs;
	_lstPollTime = other._lstPollTime;
	_isEnabled = other._isEnabled;
	_isOn = other._isOn;
	_isOnDisabled = other._isOnDisabled;
	_isPressed = other._isPressed;
	_updTmrAttchd = other._updTmrAttchd;
	_outputsChange = other._outputsChange;
	_fnWhnTrnOn = other._fnWhnTrnOn;
	_fnWhnTrnOff = other._fnWhnTrnOff;
	_fnVdPtrPrmWhnTrnOn = other._fnVdPtrPrmWhnTrnOn;
	_fnVdPtrPrmWhnTrnOff = other._fnVdPtrPrmWhnTrnOff;
	_fnVdPtrPrmWhnTrnOnArgPtr = other._fnVdPtrPrmWhnTrnOnArgPtr;
	_fnVdPtrPrmWhnTrnOffArgPtr = other._fnVdPtrPrmWhnTrnOffArgPtr;
	_dbncRlsTimeTempSett = other._dbncRlsTimeTempSett;
	_dbncTimerStrt = other._dbncTimerStrt;
	_dbncRlsTimerStrt = other._dbncRlsTimerStrt;
	_prssRlsCcl = other._prssRlsCcl;
	_validPressPend = other._validPressPend;
	_validReleasePend = other._validReleasePend;
	_validEnablePend = other._validEnablePend;
	_validDisablePend = other._validDisablePend;
	_beginDisabled = other._beginDisabled;
	_sttChng = other._sttChng;
	_mpbFdaState = other._mpbFdaState;
}

DbncdMPBttn::~DbncdMPBttn(){
    
	end();  // Stops the software timer associated to the object, deletes it's entry and nullyfies the handle to it before destructing the object
}

bool DbncdMPBttn::begin(const unsigned long int &pollDelayMs) {
	bool result {false};

	pinMode(_mpbttnPin, (_pulledUp == true)?INPUT_PULLUP:INPUT);
	if(_beginDisabled){
		_isEnabled = false;
		_validDisablePend = true;
	}

	if (pollDelayMs > 0){
		_pollPeriodMs = pollDelayMs;	// Set this MPB's PollPeriodMs to the provided argument
		_updTmrAttchd = true;	//Set the MPB object to be updated by the Timer. By manipulating the attribute (instead of ) the global _updTimerPeriod is not recalculated
		_pushMpb(_mpbsInstncsLstPtr, _mpbInstnc);	// Add the MPB to the "MPBs to be updated list"

		if (_updTimerPeriod == 0){   // The timer was not running (empty list or all listed objects not attached to the refresh)
			_updTimerPeriod = _pollPeriodMs;	//! As at this moment this is the only active MPB it's poll time is THE int time, otherwise the line would be _updTimerPeriod = _updTmrsMCDCalc();
			Timer1.attachInterrupt(_ISRMpbsRfrshCb);
			Timer1.initialize(_updTimerPeriod * 1000);	// The MPBs manages times in milliseconds, the timer in microseconds
			Timer1.start();			
		}
		else{	// The "MPBs to be updated list" was not empty, pollTime must be recalculated and if changes set Timer1.setPeriod() invoked
			if(_pollPeriodMs != _updTimerPeriod){
				_updTimerPeriod = _updTmrsMCDCalc();
				Timer1.setPeriod(_updTimerPeriod * 1000);
			}
		}
		result = true;
	}

   return result;
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

	result = pause();	//Will mark the object as non updatable, recalculate the update time and sets the timer period, or stops it if no updatable objects are left in the list.
	if (result){
		_popMpb(_mpbsInstncsLstPtr, _mpbInstnc);	// Removes the MPB from the "MPBs to be updated list". If the list is empty after the removal this method deletes the list.
		if(_mpbsInstncsLstPtr == nullptr){	// The "MPBs to be updated list" is empty, stop the Timer1
			_updTimerPeriod = 0;
			Timer1.stop();
			Timer1.detachInterrupt();
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

fncVdPtrPrmPtrType DbncdMPBttn::getFVPPWhnTrnOff(){
   
	return _fnVdPtrPrmWhnTrnOff;
}

void* DbncdMPBttn::getFVPPWhnTrnOffArgPtr(){

   return _fnVdPtrPrmWhnTrnOffArgPtr;
}

fncVdPtrPrmPtrType DbncdMPBttn::getFVPPWhnTrnOn(){

   return _fnVdPtrPrmWhnTrnOn;
}

void* DbncdMPBttn::getFVPPWhnTrnOnArgPtr(){

	return _fnVdPtrPrmWhnTrnOnArgPtr;
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

bool DbncdMPBttn::getUpdTmrAttchd(){

	return _updTmrAttchd;
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
	if(_mpbInstnc->getIsEnabled()){
		// Input/Output signals update
		_mpbInstnc->updIsPressed();
		// Flags/Triggers calculation & update
		_mpbInstnc->updValidPressesStatus();
	}
	// State machine status update
	_mpbInstnc->updFdaState();

	return;
}

uint32_t DbncdMPBttn::_otptsSttsPkg(uint32_t prevVal){
	if(_isOn)
		prevVal |= ((uint32_t)1) << IsOnBitPos;
	else
		prevVal &= ~(((uint32_t)1) << IsOnBitPos);
	if(_isEnabled)
		prevVal |= ((uint32_t)1) << IsEnabledBitPos;
	else
		prevVal &= ~(((uint32_t)1) << IsEnabledBitPos);

	return prevVal;
}

bool DbncdMPBttn::pause(){
   bool result {false};
	int arrSize{0};
	bool mpbFnd{false};

	if(_mpbsInstncsLstPtr != nullptr){	// If there is no "MPBs to be updated list", this object can't be in the list
		while(*(_mpbsInstncsLstPtr + arrSize) != nullptr){
			if(*(_mpbsInstncsLstPtr + arrSize) == _mpbInstnc){
				mpbFnd = true;
				break;
			}
			else{
				++arrSize;
			}
		}
		if(mpbFnd){	// This MPBttn was found in the "MPBs to be updated list"
			if (_updTmrAttchd == true){	// And was attached to the update timer
				_updTmrAttchd = false;
				_updTimerPeriod = _updTmrsMCDCalc();
				if(_updTimerPeriod == 0){	//No active MPBs where found in the "MPBs to be updated list", set the timer to pause
					Timer1.stop();
				}
				else{
					Timer1.setPeriod(_updTimerPeriod * 1000);
				}
			}
			result = true;
		}
	}

   return result;
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

	if(DMpbTmrUpdLst == nullptr){	// There is no "MPBs to be updated list", so create it			
		DMpbTmrUpdLst = new DbncdMPBttn* [1];
		*DMpbTmrUpdLst = nullptr;
	}

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

void DbncdMPBttn::resetDbncTime(){
   setDbncTime(_dbncTimeOrigSett);

   return; 
}

void DbncdMPBttn::resetFda(){
	clrStatus(true);
	setSttChng();
	_mpbFdaState = stOffNotVPP;

	return;
}

bool DbncdMPBttn::resume(){
	int arrSize{0};
	bool mpbFnd{false};
   bool result {false};
	unsigned long int tmpUpdTmrPrd{0};

	if(_mpbsInstncsLstPtr != nullptr){	// If there is no "MPBs to be updated list", this object can't be in the list to be resumed
		while(*(_mpbsInstncsLstPtr + arrSize) != nullptr){
			if(*(_mpbsInstncsLstPtr + arrSize) == _mpbInstnc){
				mpbFnd = true;
				break;
			}
			else{
				++arrSize;
			}
		}
		if(mpbFnd){	// This MPBttn was found in the "MPBs to be updated list"
			if (_updTmrAttchd == false){	// And it was not attached to the update timer: attach and calculate updTimerPeriod
				if(_pollPeriodMs > 0){	// The periodic polling time is a non-zero value, it can be resumed, else it fails
				   resetFda();	// To restart in a safe situation the FDA is resetted to have all flags and timers cleaned up
					_updTmrAttchd = true;
					tmpUpdTmrPrd = _updTmrsMCDCalc();
					if(_updTimerPeriod != tmpUpdTmrPrd){
						Timer1.setPeriod(tmpUpdTmrPrd * 1000);
						if(_updTimerPeriod == 0){	//No active MPBs where found in the "MPBs to be updated list", set the timer to pause
							Timer1.resume();
						}
						_updTimerPeriod = tmpUpdTmrPrd;
					}
					result = true;
				}
			}
			else{	// The object was in the "MPBs to be updated list" and was set to be updated, no need for further changes, reply success
				result = true;
			}
		}
	}

   return result;
}

void DbncdMPBttn::setBeginDisabled(const bool &newBeginDisabled){
	if(_beginDisabled != newBeginDisabled)
		_beginDisabled = newBeginDisabled;

	return;
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
	if (_fnWhnTrnOff != newFnWhnTrnOff)
		_fnWhnTrnOff = newFnWhnTrnOff;

	return;
}

void DbncdMPBttn::setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)()){
	if (_fnWhnTrnOn != newFnWhnTrnOn)
		_fnWhnTrnOn = newFnWhnTrnOn;

	return;
}

void DbncdMPBttn::setFVPPWhnTrnOff(fncVdPtrPrmPtrType newFVPPWhnTrnOff, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOff != newFVPPWhnTrnOff){
		_fnVdPtrPrmWhnTrnOff = newFVPPWhnTrnOff;
		_fnVdPtrPrmWhnTrnOffArgPtr = argPtr;
	}

	return;
}

void DbncdMPBttn::setFVPPWhnTrnOffArgPtr(void* newFVPPWhnTrnOffArgPtr){
	if (_fnVdPtrPrmWhnTrnOffArgPtr != newFVPPWhnTrnOffArgPtr)
		_fnVdPtrPrmWhnTrnOffArgPtr = newFVPPWhnTrnOffArgPtr;	

	return;
}

void DbncdMPBttn::setFVPPWhnTrnOn(fncVdPtrPrmPtrType newFVPPWhnTrnOn, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOn != newFVPPWhnTrnOn){
		_fnVdPtrPrmWhnTrnOn = newFVPPWhnTrnOn;
		_fnVdPtrPrmWhnTrnOnArgPtr = argPtr;	
	}

	return;
}

void DbncdMPBttn::setFVPPWhnTrnOnArgPtr(void* newFVPPWhnTrnOnArgPtr){
	if (_fnVdPtrPrmWhnTrnOnArgPtr != newFVPPWhnTrnOnArgPtr)
		_fnVdPtrPrmWhnTrnOnArgPtr = newFVPPWhnTrnOnArgPtr;	

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
		if(_fnVdPtrPrmWhnTrnOff != nullptr){
			_fnVdPtrPrmWhnTrnOff(_fnVdPtrPrmWhnTrnOffArgPtr);
		}
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
			if(_fnVdPtrPrmWhnTrnOn != nullptr){
				_fnVdPtrPrmWhnTrnOn(_fnVdPtrPrmWhnTrnOnArgPtr);
			}
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
				clrStatus(true);	//Uneeded as is the first function executed in the next state (stOffNotVPP), but this ensures that state can be modified without affecting this one
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

unsigned long int DbncdMPBttn::_updTmrsMCDCalc(){
   /*returning values:
      0: One of the input values was 0, or the MPBs list is empty: invalid result
      Other: This value would make the MPBs update timer save resources */
   unsigned long int MCD{0};
	int auxPtr{0};
	if(_mpbsInstncsLstPtr != nullptr){
		// The list of MPBs to be updated is not empty, there's at least one MPB
		while (*(_mpbsInstncsLstPtr + auxPtr) != nullptr){

			if ((*(_mpbsInstncsLstPtr + auxPtr))->getUpdTmrAttchd() == true){
				if(MCD == 0){
					MCD = (*(_mpbsInstncsLstPtr + auxPtr))->getPollPeriodMs();
				}
				else{
					MCD = findMCD(MCD, (*(_mpbsInstncsLstPtr + auxPtr))->getPollPeriodMs());
				}
			}
		++auxPtr;
		}
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

DbncdDlydMPBttn::DbncdDlydMPBttn()
:DbncdMPBttn()
{
}

DbncdDlydMPBttn::DbncdDlydMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett)
{
	_strtDelay = strtDelay;
}

DbncdDlydMPBttn::DbncdDlydMPBttn(const DbncdDlydMPBttn& other)
: DbncdMPBttn(other) // Call base class copy constructor
{
	this->_strtDelay = other._strtDelay;		// Copy the strtDelay attribute
}

DbncdDlydMPBttn::~DbncdDlydMPBttn()
{
}

bool DbncdDlydMPBttn::init(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay){
	bool result {false};

	result = DbncdMPBttn::init(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett);
	if (result)
		setStrtDelay(strtDelay);

	return result;
}

void DbncdDlydMPBttn::setStrtDelay(const unsigned long int &newStrtDelay){
   if(_strtDelay != newStrtDelay)
      _strtDelay = newStrtDelay;

   return;
}

//=========================================================================> Class methods delimiter

LtchMPBttn::LtchMPBttn()
{
}

LtchMPBttn::LtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DbncdDlydMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

LtchMPBttn::LtchMPBttn(const LtchMPBttn& other)	
: DbncdDlydMPBttn(other) // Call base class copy constructor
{
	_isLatched = other._isLatched;
	_trnOffASAP = other._trnOffASAP;
	_validUnlatchPend = other._validUnlatchPend;
	_validUnlatchRlsPend = other._validUnlatchRlsPend;
}

LtchMPBttn::~LtchMPBttn(){	
}

bool LtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};

	result = DbncdMPBttn::begin(pollDelayMs);

	return result;
}

void LtchMPBttn::clrStatus(bool clrIsOn){
	_isLatched = false;
	_validUnlatchPend = false;
	_validUnlatchRlsPend = false;
	DbncdMPBttn::clrStatus(clrIsOn);

	return;
}

const bool LtchMPBttn::getIsLatched() const{

	return _isLatched;
}

bool LtchMPBttn::getTrnOffASAP(){

	return _trnOffASAP;
}

const bool LtchMPBttn::getUnlatchPend() const{

	return _validUnlatchPend;
}

const bool LtchMPBttn::getUnlatchRlsPend() const{

	return _validUnlatchRlsPend;
}

void LtchMPBttn::mpbPollCallback(){

	if(_mpbInstnc->getIsEnabled()){
		// Input/Output signals update
		updIsPressed();
		// Flags/Triggers calculation & update
		updValidPressesStatus();
		updValidUnlatchStatus();
 	}
	// State machine state update
	updFdaState();

	return;
}

void LtchMPBttn::setTrnOffASAP(const bool &newVal){
	if(_trnOffASAP != newVal)
		_trnOffASAP = newVal;

	return;
}

void LtchMPBttn::setUnlatchPend(const bool &newVal){
	if(_validUnlatchPend != newVal)
		_validUnlatchPend = newVal;

	return;
}

void LtchMPBttn::setUnlatchRlsPend(const bool &newVal){
	if(_validUnlatchRlsPend != newVal)
		_validUnlatchRlsPend = newVal;

	return;
}

bool LtchMPBttn::unlatch(){
	bool result{false};

	if(_isLatched){
		setUnlatchPend(true);
		setUnlatchRlsPend(true);
		result = true;
	}

	return result;
}

void LtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				stOffNotVPP_In();
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	// For this stDisabled entry, the only flags that might be affected are _ validPressPend and (unlikely) _validReleasePend
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffNotVPP_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_isOn)
				_turnOn();
			_validPressPend = false;
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffVPP_Out();	//This function starts the latch timer here... to be considered if the MPB release must be the starting point Gaby
			}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stOnNVRP_Do();
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
			_validReleasePend = false;
			if(!_isLatched)
				_isLatched = true;
			_mpbFdaState = stLtchNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stLtchNVUP:	//From this state on different unlatch sources might make sense
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stLtchNVUP_Do();
			if(_validUnlatchPend){
				_mpbFdaState = stLtchdVUP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;
				setSttChng();	//Set flag to execute exiting OUT code
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stLtchdVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_trnOffASAP){
				if(_isOn)
					_turnOff();
			}
			_mpbFdaState = stOffVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOffVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validUnlatchPend = false;	// This is a placeholder for updValidUnlatchStatus() implemented in each subclass
			_mpbFdaState = stOffNVURP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOffNVURP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validUnlatchRlsPend){
				_mpbFdaState = stOffVURP;
				setSttChng();
			}
			stOffNVURP_Do();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVURP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validUnlatchRlsPend = false;
			if(_isOn)
				_turnOff();
			if(_isLatched)
				_isLatched = false;
			if(_validPressPend)
				_validPressPend = false;
			if(_validReleasePend)
				_validReleasePend = false;
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOffVURP_Out();
			}	// Execute this code only ONCE, when exiting this state
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
				stDisabled_In();
				_validDisablePend = false;
				_isEnabled = false;
				setOutputsChange(true);
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				if(_isOn)
					_turnOff();
				_isEnabled = true;
				_validEnablePend = false;
				setOutputsChange(true);
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				clrStatus(true);
				stDisabled_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

	default:
		break;
	}

	return;
}

//=========================================================================> Class methods delimiter

TgglLtchMPBttn::TgglLtchMPBttn()
{
}

TgglLtchMPBttn::TgglLtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

TgglLtchMPBttn::TgglLtchMPBttn(const TgglLtchMPBttn& other)
: LtchMPBttn(other)
{
	// No additional members to initialize beyond base class constructor parameters
}

TgglLtchMPBttn::~TgglLtchMPBttn(){	
}

void TgglLtchMPBttn::stOffNVURP_Do(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	if(_validDisablePend){
		if(_validUnlatchRlsPend)
			_validUnlatchRlsPend = false;
		_mpbFdaState = stDisabled;
		setSttChng();	//Set flag to execute exiting OUT code
	}

	return;
}

void TgglLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			_validUnlatchPend = true;
			_validPressPend = false;
		}
		if(_validReleasePend){
			_validUnlatchRlsPend = true;
			_validReleasePend = false;
		}
	}

	return;
}

//=========================================================================> Class methods delimiter

TmLtchMPBttn::TmLtchMPBttn()
{
}

TmLtchMPBttn::TmLtchMPBttn(const uint8_t &mpbttnPin, const unsigned long int &actTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _srvcTime{actTime}
{
	if(_srvcTime < _MinSrvcTime)    //Best practice would impose failing the constructor (throwing an exception or building a "zombie" object)
		_srvcTime = _MinSrvcTime;    //this tolerant approach taken for developers benefit, but object will be no faithful to the instantiation parameters

}

TmLtchMPBttn::TmLtchMPBttn(const TmLtchMPBttn &other)
: LtchMPBttn(other), _srvcTime{other._srvcTime}
{
	this->_srvcTimerStrt = other._srvcTimerStrt;
	this->_tmRstbl = other._tmRstbl;
}

TmLtchMPBttn::~TmLtchMPBttn()
{	
}

void TmLtchMPBttn::clrStatus(bool clrIsOn){
	_srvcTimerStrt = 0;
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

const unsigned long int TmLtchMPBttn::getSrvcTime() const{

	return _srvcTime;
}

bool TmLtchMPBttn::setSrvcTime(const unsigned long int &newSrvcTime){
	bool result {true};

	if (_srvcTime != newSrvcTime){
		if (newSrvcTime >= _MinSrvcTime)  //The minimum activation time is _minActTime milliseconds
			_srvcTime = newSrvcTime;
		else
			result = false;
   }

   return result;
}

void TmLtchMPBttn::setTmerRstbl(const bool &newIsRstbl){
	if(_tmRstbl != newIsRstbl)
		_tmRstbl = newIsRstbl;

	return;
}

void TmLtchMPBttn::stOffNotVPP_Out(){
	_srvcTimerStrt = 0;

	return;
}

void TmLtchMPBttn::stOffVPP_Out(){
	_srvcTimerStrt = millis();

	return;
}

void TmLtchMPBttn::updValidUnlatchStatus(){
	if(_isLatched){
		if(_validPressPend){
			if(_tmRstbl)
				_srvcTimerStrt = millis();
			_validPressPend = false;
		}
		if ((millis() - _srvcTimerStrt) >= _srvcTime){
			_validUnlatchPend = true;
			_validUnlatchRlsPend = true;
		}
	}

	return;
}

//=========================================================================> Class methods delimiter

HntdTmLtchMPBttn::HntdTmLtchMPBttn()
{
}

HntdTmLtchMPBttn::HntdTmLtchMPBttn(const uint8_t &mpbttnPin, const unsigned long int &actTime, const unsigned int &wrnngPrctg, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:TmLtchMPBttn(mpbttnPin, actTime, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _wrnngPrctg{wrnngPrctg}
{
	_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;   
}

HntdTmLtchMPBttn::HntdTmLtchMPBttn(const HntdTmLtchMPBttn &other)
: TmLtchMPBttn(other), _wrnngPrctg{other._wrnngPrctg}
{
	_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;
	_fnWhnTrnOffPilot = other._fnWhnTrnOffPilot;
	_fnWhnTrnOffWrnng = other._fnWhnTrnOffWrnng;
	_fnWhnTrnOnPilot = other._fnWhnTrnOnPilot;
	_fnWhnTrnOnWrnng = other._fnWhnTrnOnWrnng;
	_keepPilot = other._keepPilot;
	_pilotOn = other._pilotOn;
	_wrnngOn = other._wrnngOn;
	_validWrnngSetPend = other._validWrnngSetPend;
	_validWrnngResetPend = other._validWrnngResetPend;
	_validPilotSetPend = other._validPilotSetPend;
	_validPilotResetPend = other._validPilotResetPend;
}

HntdTmLtchMPBttn::~HntdTmLtchMPBttn()
{	
}

bool HntdTmLtchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};

	result = LtchMPBttn::begin(pollDelayMs);

	return result;
}

void HntdTmLtchMPBttn::clrStatus(bool clrIsOn){
	
	//	Put here class specific sets/resets, including pilot and warning
	_validWrnngSetPend = false;
	_validWrnngResetPend = false;
	_wrnngOn = false; // Direct attribute flag unusual manipulation to avoid triggering Tasks and Functions responses
	_validPilotSetPend = false;
	_validPilotResetPend = false;
	if(_keepPilot)
		_pilotOn = true; // Direct attribute flag unusual manipulation to avoid triggering Tasks and Functions responses
	else
		_pilotOn = false; // Direct attribute flag unusual manipulation to avoid triggering Tasks and Functions responses
	TmLtchMPBttn::clrStatus(clrIsOn);

	return;
}

fncPtrType HntdTmLtchMPBttn::getFnWhnTrnOffPilot(){

	return _fnWhnTrnOffPilot;
}

fncPtrType HntdTmLtchMPBttn::getFnWhnTrnOffWrnng(){

	return _fnWhnTrnOffWrnng;
}

fncPtrType HntdTmLtchMPBttn::getFnWhnTrnOnPilot(){

	return _fnWhnTrnOnPilot;
}

fncPtrType HntdTmLtchMPBttn::getFnWhnTrnOnWrnng(){
	
	return _fnWhnTrnOnWrnng;
}

fncVdPtrPrmPtrType HntdTmLtchMPBttn::getFVPPWhnTrnOffPilot(){
	
   return _fnVdPtrPrmWhnTrnOffPilot;
}

void* HntdTmLtchMPBttn::getFVPPWhnTrnOffPilotArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOffPilotArgPtr;
}

fncVdPtrPrmPtrType HntdTmLtchMPBttn::getFVPPWhnTrnOnPilot(){
	
	return _fnVdPtrPrmWhnTrnOnPilot;
}

void* HntdTmLtchMPBttn::getFVPPWhnTrnOnPilotArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOnPilotArgPtr;
}

fncVdPtrPrmPtrType HntdTmLtchMPBttn::getFVPPWhnTrnOffWrnng(){
	
	return _fnVdPtrPrmWhnTrnOffWrnng;
}

void* HntdTmLtchMPBttn::getFVPPWhnTrnOffWrnngArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOffWrnngArgPtr;
}

fncVdPtrPrmPtrType HntdTmLtchMPBttn::getFVPPWhnTrnOnWrnng(){
	
	return _fnVdPtrPrmWhnTrnOnWrnng;
}

void* HntdTmLtchMPBttn::getFVPPWhnTrnOnWrnngArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOnWrnngArgPtr;
}

const bool HntdTmLtchMPBttn::getPilotOn() const{

	return _pilotOn;
}

const bool HntdTmLtchMPBttn::getWrnngOn() const{
    
	return _wrnngOn;
}

void HntdTmLtchMPBttn::mpbPollCallback(){
	if(_mpbInstnc->getIsEnabled()){
		// Input/Output signals update
		updIsPressed();
		// Flags/Triggers calculation & update
		updValidPressesStatus();
		updValidUnlatchStatus();
		updWrnngOn();
		updPilotOn();
	}
 	// State machine state update
 	updFdaState();

	return;
}

uint32_t HntdTmLtchMPBttn::_otptsSttsPkg(uint32_t prevVal){
	prevVal = DbncdMPBttn::_otptsSttsPkg(prevVal);
	if(_pilotOn){
		prevVal |= ((uint32_t)1) << PilotOnBitPos;
	}
	else{
		prevVal &= ~(((uint32_t)1) << PilotOnBitPos);
	}
	if(_wrnngOn){
		prevVal |= ((uint32_t)1) << WrnngOnBitPos;
	}
	else{
		prevVal &= ~(((uint32_t)1) << WrnngOnBitPos);
	}

	return prevVal;
}

void HntdTmLtchMPBttn::setFnWhnTrnOffPilotPtr(void(*newFnWhnTrnOff)()){
	if (_fnWhnTrnOffPilot != newFnWhnTrnOff)
		_fnWhnTrnOffPilot = newFnWhnTrnOff;

	return;	
}

void HntdTmLtchMPBttn::setFnWhnTrnOffWrnngPtr(void(*newFnWhnTrnOff)()){
	if (_fnWhnTrnOffWrnng != newFnWhnTrnOff)
		_fnWhnTrnOffWrnng = newFnWhnTrnOff;

	return;
}

void HntdTmLtchMPBttn::setFnWhnTrnOnPilotPtr(void(*newFnWhnTrnOn)()){
	if (_fnWhnTrnOnPilot != newFnWhnTrnOn)
		_fnWhnTrnOnPilot = newFnWhnTrnOn;

	return;
}

void HntdTmLtchMPBttn::setFnWhnTrnOnWrnngPtr(void(*newFnWhnTrnOn)()){
	if (_fnWhnTrnOnWrnng != newFnWhnTrnOn)
		_fnWhnTrnOnWrnng = newFnWhnTrnOn;

	return;
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOffPilot(fncVdPtrPrmPtrType newFVPPWhnTrnOff, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOffPilot != newFVPPWhnTrnOff){
		_fnVdPtrPrmWhnTrnOffPilot = newFVPPWhnTrnOff;
		_fnVdPtrPrmWhnTrnOffPilotArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOffPilotArgPtr(void *argPtr){
	if (_fnVdPtrPrmWhnTrnOffPilotArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOffPilotArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOnPilot(fncVdPtrPrmPtrType newFVPPWhnTrnOn, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOnPilot != newFVPPWhnTrnOn){
		_fnVdPtrPrmWhnTrnOnPilot = newFVPPWhnTrnOn;
		_fnVdPtrPrmWhnTrnOnPilotArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOnPilotArgPtr(void *argPtr){
	if (_fnVdPtrPrmWhnTrnOnPilotArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOnPilotArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOffWrnng(fncVdPtrPrmPtrType newFVPPWhnTrnOff, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOffWrnng != newFVPPWhnTrnOff){
		_fnVdPtrPrmWhnTrnOffWrnng = newFVPPWhnTrnOff;
		_fnVdPtrPrmWhnTrnOffWrnngArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOffWrnngArgPtr(void *argPtr){
	if (_fnVdPtrPrmWhnTrnOffWrnngArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOffWrnngArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOnWrnng(fncVdPtrPrmPtrType newFVPPWhnTrnOn, void *argPtr){
	if (_fnVdPtrPrmWhnTrnOnWrnng != newFVPPWhnTrnOn){
		_fnVdPtrPrmWhnTrnOnWrnng = newFVPPWhnTrnOn;
		_fnVdPtrPrmWhnTrnOnWrnngArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setFVPPWhnTrnOnWrnngArgPtr(void *argPtr){
	if (_fnVdPtrPrmWhnTrnOnWrnngArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOnWrnngArgPtr = argPtr;
	}

	return;	
}

void HntdTmLtchMPBttn::setKeepPilot(const bool &newKeepPilot){
	if(_keepPilot != newKeepPilot)
		_keepPilot = newKeepPilot;

	return;
}

bool HntdTmLtchMPBttn::setSrvcTime(const unsigned long int &newSrvcTime){
	bool result {true};

	if (newSrvcTime != _srvcTime){
		result = TmLtchMPBttn::setSrvcTime(newSrvcTime);
		if (result)
			_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;  //If the _srvcTime was changed, the _wrnngMs must be updated as it's a percentage of the first
	}

	return result;
}

bool HntdTmLtchMPBttn::setWrnngPrctg (const unsigned int &newWrnngPrctg){
	bool result{false};

	if(_wrnngPrctg != newWrnngPrctg){
		if(newWrnngPrctg <= 100){
			_wrnngPrctg = newWrnngPrctg;
			_wrnngMs = (_srvcTime * _wrnngPrctg) / 100;
			result = true;
		}
	}

	return result;
}

void HntdTmLtchMPBttn::stDisabled_In(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	if(_validWrnngSetPend)
		_validWrnngSetPend = false;
	if(_validWrnngResetPend)
		_validWrnngResetPend = false;
	if(_wrnngOn){
		_turnOffWrnng();
	}

	if(_validPilotSetPend)
		_validPilotSetPend = false;
	if(_validPilotResetPend)
		_validPilotResetPend = false;
	if(_keepPilot && !_isOnDisabled && !_pilotOn){
		_turnOnPilot();
	}
	else if(_pilotOn == true)
		_turnOffPilot();

	return;
}

void HntdTmLtchMPBttn::stLtchNVUP_Do(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	if(_validWrnngSetPend){
		_turnOnWrnng();
		_validWrnngSetPend = false;
	}
	if(_validWrnngResetPend){
		_turnOffWrnng();
		_validWrnngResetPend = false;
	}

	return;
}

void HntdTmLtchMPBttn::stOffNotVPP_In(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	if(_keepPilot){
		if(!_pilotOn){
			_turnOnPilot();
		}
	}
	if(_wrnngOn){
		_turnOffWrnng();
	}

	return;
}

void HntdTmLtchMPBttn::stOffVPP_Out(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	TmLtchMPBttn::stOffVPP_Out();
	if(_pilotOn){
		_turnOffPilot();
	}

	return;
}

void HntdTmLtchMPBttn::stOnNVRP_Do(){
	//This method is invoked exclusively from the updFdaState, no need to declare it critical section
	if(_validWrnngSetPend){
		_turnOnWrnng();
		_validWrnngSetPend = false;
	}
	if(_validWrnngResetPend){
		_turnOffWrnng();
		_validWrnngResetPend = false;
	}

	return;
}
	
void HntdTmLtchMPBttn::_turnOffPilot(){
	if(_pilotOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffPilot != nullptr){
			_fnWhnTrnOffPilot();
		}
		if(_fnVdPtrPrmWhnTrnOffPilot != nullptr){
			_fnVdPtrPrmWhnTrnOffPilot(_fnVdPtrPrmWhnTrnOffPilotArgPtr);
		}
		//---------------->> Flags related actions
		_pilotOn = false;
		setOutputsChange(true);
	}

	return;
}

void HntdTmLtchMPBttn::_turnOffWrnng(){
	if(_wrnngOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffWrnng != nullptr){
			_fnWhnTrnOffWrnng();
		}
		if(_fnVdPtrPrmWhnTrnOffWrnng != nullptr){
			_fnVdPtrPrmWhnTrnOffWrnng(_fnVdPtrPrmWhnTrnOffWrnngArgPtr);
		}
		//---------------->> Flags related actions
		_wrnngOn = false;
		setOutputsChange(true);
	}

	return;
}

void HntdTmLtchMPBttn::_turnOnPilot(){
	if(!_pilotOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnPilot != nullptr){
			_fnWhnTrnOnPilot();
		}
		if(_fnVdPtrPrmWhnTrnOnPilot != nullptr){
			_fnVdPtrPrmWhnTrnOnPilot(_fnVdPtrPrmWhnTrnOnPilotArgPtr);
		}
		//---------------->> Flags related actions
		_pilotOn = true;
		setOutputsChange(true);
	}

	return;
}

void HntdTmLtchMPBttn::_turnOnWrnng(){
	if(!_wrnngOn){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnWrnng != nullptr){
			_fnWhnTrnOnWrnng();
		}
		if(_fnVdPtrPrmWhnTrnOnWrnng != nullptr){
			_fnVdPtrPrmWhnTrnOnWrnng(_fnVdPtrPrmWhnTrnOnWrnngArgPtr);
		}
		//---------------->> Flags related actions
		_wrnngOn = true;
		setOutputsChange(true);
	}

	return;
}

bool HntdTmLtchMPBttn::updPilotOn(){
	if (_keepPilot){
		if(!_isOn && !_pilotOn){
			_validPilotSetPend = true;
			_validPilotResetPend = false;
		}
		else if(_isOn && _pilotOn){
			_validPilotResetPend = true;
			_validPilotSetPend = false;
		}
	}
	else{
		if(_pilotOn){
			_validPilotResetPend = true;
			_validPilotSetPend = false;
		}
	}

	return _pilotOn;
}

bool HntdTmLtchMPBttn::updWrnngOn(){
	if(_wrnngPrctg > 0){
		if (_isOn && _isEnabled){	//The _isEnabled evaluation is done to avoid computation of flags that will be ignored if the MPB is disablee
			if ((millis() - _srvcTimerStrt) >= (_srvcTime - _wrnngMs)){
				if(_wrnngOn == false){
					_validWrnngSetPend = true;
					_validWrnngResetPend = false;
				}
			}
			else if(_wrnngOn == true){
				_validWrnngResetPend = true;
				_validWrnngSetPend = false;
			}
		}
		else if(_wrnngOn == true){
			_validWrnngResetPend = true;
			_validWrnngSetPend = false;
		}
	}

	return _wrnngOn;
}

//=========================================================================> Class methods delimiter

XtrnUnltchMPBttn::XtrnUnltchMPBttn()
{	
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(const uint8_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _unLtchBttn{unLtchBttn}
{
}

XtrnUnltchMPBttn::XtrnUnltchMPBttn(const uint8_t &mpbttnPin,  
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

bool XtrnUnltchMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};

	result = LtchMPBttn::begin(pollDelayMs);

	if (result){
		if(_unLtchBttn != nullptr)
			result = _unLtchBttn->begin();
		else
			result = true;
	}


	return result;
}

void XtrnUnltchMPBttn::clrStatus(bool clrIsOn){
	_xtrnUnltchPRlsCcl = false;
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

void XtrnUnltchMPBttn::stOffNVURP_Do(){
	if(_validDisablePend){
		if(_validUnlatchRlsPend)
			_validUnlatchRlsPend = false;
		if(_xtrnUnltchPRlsCcl)
			_xtrnUnltchPRlsCcl = false;
		_mpbFdaState = stDisabled;
		setSttChng();
	}

	return;
}

void XtrnUnltchMPBttn::updValidUnlatchStatus(){
	if(_unLtchBttn != nullptr){
		if(_isLatched){
			if (_unLtchBttn->getIsOn() && !_xtrnUnltchPRlsCcl){
				_validUnlatchPend = true;
				_xtrnUnltchPRlsCcl = true;
			}
			if(!_unLtchBttn->getIsOn() && _xtrnUnltchPRlsCcl){
				_validUnlatchRlsPend = true;
				_xtrnUnltchPRlsCcl = false;
			}
		}
		else{
			if(_xtrnUnltchPRlsCcl)
				_xtrnUnltchPRlsCcl = false;
		}
	}
	
	return;
}

//=========================================================================> Class methods delimiter

DblActnLtchMPBttn::DblActnLtchMPBttn()
{
}

DblActnLtchMPBttn::DblActnLtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:LtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DblActnLtchMPBttn::~DblActnLtchMPBttn()
{
}

bool DblActnLtchMPBttn::begin(const unsigned long int &pollDelayMs) {
	bool result{false};

	result = LtchMPBttn::begin(pollDelayMs);

	return result;
}

void DblActnLtchMPBttn::clrStatus(bool clrIsOn){
	_scndModTmrStrt = 0;
	_validScndModPend = false;
	if(clrIsOn && _isOnScndry)
		_turnOffScndry();
	LtchMPBttn::clrStatus(clrIsOn);

	return;
}

fncPtrType DblActnLtchMPBttn::getFnWhnTrnOffScndry(){

	return _fnWhnTrnOffScndry;
}

fncPtrType DblActnLtchMPBttn::getFnWhnTrnOnScndry(){

	return _fnWhnTrnOnScndry;
}

fncVdPtrPrmPtrType DblActnLtchMPBttn::getFVPPWhnTrnOffScndry(){
	
	return _fnVdPtrPrmWhnTrnOffScndry;
}

void* DblActnLtchMPBttn::getFVPPWhnTrnOffScndryArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOffScndryArgPtr;
}

fncVdPtrPrmPtrType DblActnLtchMPBttn::getFVPPWhnTrnOnScndry(){
	
	return _fnVdPtrPrmWhnTrnOnScndry;
}

void* DblActnLtchMPBttn::getFVPPWhnTrnOnScndryArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOnScndryArgPtr;
}

bool DblActnLtchMPBttn::getIsOnScndry(){

	return _isOnScndry;
}

unsigned long DblActnLtchMPBttn::getScndModActvDly(){

	return _scndModActvDly;
}

void DblActnLtchMPBttn::mpbPollCallback(){
	if(_mpbInstnc->getIsEnabled()){
		// Input/Output signals update
		updIsPressed();
		// Flags/Triggers calculation & update
		updValidPressesStatus();
	}
 	// State machine state update
	updFdaState();

	return;
}

uint32_t DblActnLtchMPBttn::_otptsSttsPkg(uint32_t prevVal){
	prevVal = DbncdMPBttn::_otptsSttsPkg(prevVal);
	if(_isOnScndry)
		prevVal |= ((uint32_t)1) << IsOnScndryBitPos;
	else
		prevVal &= ~(((uint32_t)1) << IsOnScndryBitPos);

	return prevVal;
}

void DblActnLtchMPBttn::setFnWhnTrnOffScndryPtr(void (*newFnWhnTrnOff)()){
	if (_fnWhnTrnOffScndry != newFnWhnTrnOff){
		_fnWhnTrnOffScndry = newFnWhnTrnOff;
	}

	return;
}

void DblActnLtchMPBttn::setFnWhnTrnOnScndryPtr(void (*newFnWhnTrnOn)()){
	if (_fnWhnTrnOnScndry != newFnWhnTrnOn)
		_fnWhnTrnOnScndry = newFnWhnTrnOn;

	return;
}

void DblActnLtchMPBttn::setFVPPWhnTrnOffScndry(fncVdPtrPrmPtrType newFVPPWhnTrnOff, void* argPtr){
	if (_fnVdPtrPrmWhnTrnOffScndry != newFVPPWhnTrnOff){
		_fnVdPtrPrmWhnTrnOffScndry = newFVPPWhnTrnOff;
		_fnVdPtrPrmWhnTrnOffScndryArgPtr = argPtr;
	}

	return;	
}

void DblActnLtchMPBttn::setFVPPWhnTrnOffScndryArgPtr(void* argPtr){
	if (_fnVdPtrPrmWhnTrnOffScndryArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOffScndryArgPtr = argPtr;
	}

	return;	
}

void DblActnLtchMPBttn::setFVPPWhnTrnOnScndry(fncVdPtrPrmPtrType newFVPPWhnTrnOn, void* argPtr){
	if (_fnVdPtrPrmWhnTrnOnScndry != newFVPPWhnTrnOn){
		_fnVdPtrPrmWhnTrnOnScndry = newFVPPWhnTrnOn;
		_fnVdPtrPrmWhnTrnOnScndryArgPtr = argPtr;
	}

	return;	
}

void DblActnLtchMPBttn::setFVPPWhnTrnOnScndryArgPtr(void* argPtr){
	if (_fnVdPtrPrmWhnTrnOnScndryArgPtr != argPtr){
		_fnVdPtrPrmWhnTrnOnScndryArgPtr = argPtr;
	}

	return;	
}

bool DblActnLtchMPBttn::setScndModActvDly(const unsigned long &newVal){
	bool result {true};

	if(newVal != _scndModActvDly){
		if (newVal >= _MinSrvcTime){  //The minimum activation time is _minActTime
			_scndModActvDly = newVal;
		}
		else{
			result = false;
		}
	}

	return result;
}

void DblActnLtchMPBttn::_turnOffScndry(){

	if(_isOnScndry){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffScndry != nullptr)
			_fnWhnTrnOffScndry();
		if(_fnVdPtrPrmWhnTrnOffScndry != nullptr){
			_fnVdPtrPrmWhnTrnOffScndry(_fnVdPtrPrmWhnTrnOffScndryArgPtr);
		}
		//---------------->> Flags related actions
		_isOnScndry = false;
		setOutputsChange(true);
	}

	return;
}

void DblActnLtchMPBttn::_turnOnScndry(){

	if(!_isOnScndry){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnScndry != nullptr)
			_fnWhnTrnOnScndry();
		if(_fnVdPtrPrmWhnTrnOnScndry != nullptr){
			_fnVdPtrPrmWhnTrnOnScndry(_fnVdPtrPrmWhnTrnOnScndryArgPtr);
		}
		//---------------->> Flags related actions
		_isOnScndry = true;
		setOutputsChange(true);
	}

	return;
}

void DblActnLtchMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend || _validScndModPend){
				_mpbFdaState = stOffVPP;	//Start pressing timer
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
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
			if(_validScndModPend){
				_scndModTmrStrt = millis();	//>Gaby is this needed after separating this class from the sldr... class? Better do a subclass function!
				_mpbFdaState = stOnStrtScndMod;
				setSttChng();
			}
			else if(_validPressPend && _validReleasePend){
				_validPressPend = false;
				_validReleasePend = false;
				_mpbFdaState = stOnMPBRlsd;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnStrtScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){
				stOnStrtScndMod_In();
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOnScndMod;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(!_validReleasePend){
				//Operating in Second Mode
				stOnScndMod_Do();
			}
			else{
				// MPB released, close Slider mode, move on to next state
				_mpbFdaState = stOnEndScndMod;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnEndScndMod:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_scndModTmrStrt = 0;
			_validScndModPend = false;
			_mpbFdaState = stOnMPBRlsd;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){
				stOnEndScndMod_Out();
			}
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnMPBRlsd:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validScndModPend){
				_scndModTmrStrt = millis();
				_mpbFdaState = stOnStrtScndMod;
				setSttChng();
			}
			else if(_validPressPend && _validReleasePend){
				_validPressPend = false;
				_validReleasePend = false;
				_mpbFdaState = stOnTurnOff;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnTurnOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				stDisabled_In();
				if(_isOn != _isOnDisabled){
					if(_isOn)
						_turnOff();
					else
						_turnOn();
				}
				if(_isOnScndry != _isOnDisabled){
					if(_isOnScndry)
						_turnOffScndry();
					else
						_turnOnScndry();
				}
				clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected
				_isEnabled = false;
				_validDisablePend = false;
				setOutputsChange(true);
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				if(_isOnScndry)
					_turnOffScndry();
				if(_isOn)
					_turnOff();
				_isEnabled = true;
				_validEnablePend = false;
				setOutputsChange(true);
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

	return;
}

bool DblActnLtchMPBttn::updValidPressesStatus(){
	if(_isPressed){
		if(_dbncRlsTimerStrt != 0)
			_dbncRlsTimerStrt = 0;
		if(_dbncTimerStrt == 0){    //It was not previously pressed
			_dbncTimerStrt = millis();	//Started to be pressed
		}
		else{
			if ((millis() - _dbncTimerStrt) >= ((_dbncTimeTempSett + _strtDelay) + _scndModActvDly)){
				_validScndModPend = true;
				_validPressPend = false;
			} else if ((millis() - _dbncTimerStrt) >= (_dbncTimeTempSett + _strtDelay)){
				_validPressPend = true;
			}
			if(_validPressPend || _validScndModPend){
				_validReleasePend = false;
				_prssRlsCcl = true;
			}
		}
	}
	else{
		if(_dbncTimerStrt != 0)
			_dbncTimerStrt = 0;
		if(!_validReleasePend && _prssRlsCcl){
			if(_dbncRlsTimerStrt == 0){    //It was not previously pressed
				_dbncRlsTimerStrt = millis();	//Started to be UNpressed
			}
			else{
				if ((millis() - _dbncRlsTimerStrt) >= (_dbncRlsTimeTempSett)){
					_validReleasePend = true;
					_prssRlsCcl = false;
				}
			}
		}
	}

	return (_validPressPend || _validScndModPend);
}

void DblActnLtchMPBttn::updValidUnlatchStatus(){
	_validUnlatchPend = true;

	return;
}

//=========================================================================> Class methods delimiter

DDlydDALtchMPBttn::DDlydDALtchMPBttn()
{	
}

DDlydDALtchMPBttn::DDlydDALtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:DblActnLtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
}

DDlydDALtchMPBttn::~DDlydDALtchMPBttn()
{
}

void DDlydDALtchMPBttn::clrStatus(bool clrIsOn){
	if(clrIsOn && _isOnScndry){
		_turnOffScndry();
	}
	DblActnLtchMPBttn::clrStatus(clrIsOn);

	return;
}

/*void DDlydDALtchMPBttn::stDisabled_In(){	
	if(_isOnScndry != _isOnDisabled){
		if(_isOnDisabled)
			_turnOnScndry();
		else
			_turnOffScndry();
	}

	return;
}*/

void DDlydDALtchMPBttn::stOnEndScndMod_Out(){
	if(_isOnScndry)
		_turnOffScndry();

	return;
}

void DDlydDALtchMPBttn::stOnScndMod_Do(){

	return;
}

void DDlydDALtchMPBttn::stOnStrtScndMod_In(){
	if(!_isOnScndry)
		_turnOnScndry();

	return;
}

//=========================================================================> Class methods delimiter

SldrDALtchMPBttn::SldrDALtchMPBttn()
{	
}

SldrDALtchMPBttn::SldrDALtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const uint16_t initVal)
:DblActnLtchMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay), _initOtptCurVal{initVal}
{
	_otptCurVal = _initOtptCurVal;
}

SldrDALtchMPBttn::~SldrDALtchMPBttn()
{
}

void SldrDALtchMPBttn::clrStatus(bool clrIsOn){
	// Might the option to return the _otpCurVal to the initVal be added? To one the extreme values?
	if(clrIsOn && _isOnScndry)
		_turnOffScndry();
	DblActnLtchMPBttn::clrStatus(clrIsOn);

	return;
}

uint16_t SldrDALtchMPBttn::getOtptCurVal(){

	return _otptCurVal;
}

bool SldrDALtchMPBttn::getOtptCurValIsMax(){

	// return (_otptCurVal == _otptValMax);
	return _otptCurValIsMax;
}

bool SldrDALtchMPBttn::getOtptCurValIsMin(){

	// return (_otptCurVal == _otptValMin);
	return _otptCurValIsMin;
}

unsigned long SldrDALtchMPBttn::getOtptSldrSpd(){

	return _otptSldrSpd;
}

uint16_t SldrDALtchMPBttn::getOtptSldrStpSize(){

	return _otptSldrStpSize;
}

uint16_t SldrDALtchMPBttn::getOtptValMax(){

	return _otptValMax;
}

uint16_t SldrDALtchMPBttn::getOtptValMin(){

	return _otptValMin;
}

bool SldrDALtchMPBttn::getSldrDirUp(){

	return _curSldrDirUp;
}

uint32_t SldrDALtchMPBttn::_otptsSttsPkg(uint32_t prevVal){
	prevVal = DblActnLtchMPBttn::_otptsSttsPkg(prevVal);
	prevVal |= (((uint32_t)_otptCurVal) << OtptCurValBitPos);

	return prevVal;
}

bool SldrDALtchMPBttn::setOtptCurVal(const uint16_t &newVal){
	bool result{true};

	if(_otptCurVal != newVal){
		if(newVal >= _otptValMin && newVal <= _otptValMax)
			_otptCurVal = newVal;
		else
			result = false;
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptSldrSpd(const uint16_t &newVal){
	bool result{true};

	if(newVal != _otptSldrSpd){
		if(newVal > 0)
			_otptSldrSpd = newVal;
		else
			result = false;
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptSldrStpSize(const uint16_t &newVal){
	bool result{true};

	if(newVal != _otptSldrStpSize){
		if((newVal > 0) && (newVal <= (_otptValMax - _otptValMin) / _otptSldrSpd))	//If newVal == (_otptValMax - _otptValMin) the slider will work as kind of an On/Off switch
			_otptSldrStpSize = newVal;
		else
			result = false;
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptValMax(const uint16_t &newVal){
	bool result{true};

	if(newVal != _otptValMax){
		if(newVal > _otptValMin){
			_otptValMax = newVal;
			if(_otptCurVal > _otptValMax){
				_otptCurVal = _otptValMax;
				setOutputsChange(true);
			}
		}
		else{
			result = false;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::setOtptValMin(const uint16_t &newVal){
	bool result{true};

	if(newVal != _otptValMin){
		if(newVal < _otptValMax){
			_otptValMin = newVal;
			if(_otptCurVal < _otptValMin){
				_otptCurVal = _otptValMin;
				setOutputsChange(true);
			}
		}
		else{
			result = false;
		}
	}

	return result;
}

bool SldrDALtchMPBttn::_setSldrDir(const bool &newVal){
	bool result{true};

	if(newVal != _curSldrDirUp){
		if(newVal){	//Try to set new direction Up
			if(_otptCurVal != _otptValMax)
				_curSldrDirUp = true;
		}
		else{		//Try to set new direction down
			if(_otptCurVal != _otptValMin)
				_curSldrDirUp = false;
		}
		if(_curSldrDirUp != newVal)
			result = false;
	}

	return result;
}

bool SldrDALtchMPBttn::setSldrDirDn(){

	return _setSldrDir(false);
}

bool SldrDALtchMPBttn::setSldrDirUp(){

	return _setSldrDir(true);
}

void SldrDALtchMPBttn::setSwpDirOnEnd(const bool &newVal){
	if(_autoSwpDirOnEnd != newVal)
		_autoSwpDirOnEnd = newVal;

	return;
}

void SldrDALtchMPBttn::setSwpDirOnPrss(const bool &newVal){
	if(_autoSwpDirOnPrss != newVal)
		_autoSwpDirOnPrss = newVal;

	return;
}

/*void SldrDALtchMPBttn::stDisabled_In(){
	if(_isOnScndry != _isOnDisabled){
		if(_isOnDisabled)
			_turnOnScndry();
		else
			_turnOffScndry();
	}

	return;
}*/

void SldrDALtchMPBttn::stOnEndScndMod_Out(){
	if(_isOnScndry)
		_turnOffScndry();

	return;
}

void SldrDALtchMPBttn::stOnScndMod_Do(){
	// Operating in Slider mode, change the associated value according to the time elapsed since last update
	//and the step size for every time unit elapsed
	uint16_t _otpStpsChng{0};
	unsigned long _sldrTmrNxtStrt{0};
	unsigned long _sldrTmrRemains{0};

	_sldrTmrNxtStrt = millis();
	_otpStpsChng = (_sldrTmrNxtStrt - _scndModTmrStrt) /_otptSldrSpd;
	_sldrTmrRemains = ((_sldrTmrNxtStrt - _scndModTmrStrt) % _otptSldrSpd) * _otptSldrSpd;
	_sldrTmrNxtStrt -= _sldrTmrRemains;
	_scndModTmrStrt = _sldrTmrNxtStrt;	//This ends the time management section of the state, calculating the time

	if(_curSldrDirUp){	// The slider is moving up		
		if(_otptCurVal != _otptValMax){
			if((_otptValMax - _otptCurVal) >= (_otpStpsChng * _otptSldrStpSize)){	//The value change is in range				
				_otptCurVal += (_otpStpsChng * _otptSldrStpSize);
			}
			else{	//The value change goes out of range				
				_otptCurVal = _otptValMax;
			}
			setOutputsChange(true);
		}
		if(getOutputsChange()){
			if(_otptCurValIsMin){
				_turnOffSldrMin();
			}
			if(_otptCurVal == _otptValMax){
				_turnOnSldrMax();
				if(_autoSwpDirOnEnd == true){
					_curSldrDirUp = false;
				}
			}
		}
	}
	else{	// The slider is moving down		
		if(_otptCurVal != _otptValMin){
			if((_otptCurVal - _otptValMin) >= (_otpStpsChng * _otptSldrStpSize)){	//The value change is in range				
				_otptCurVal -= (_otpStpsChng * _otptSldrStpSize);
			}
			else{	//The value change goes out of range				
				_otptCurVal = _otptValMin;
			}
			setOutputsChange(true);
		}
		if(getOutputsChange()){
			if(_otptCurValIsMax){
				_turnOffSldrMax();
			}
			if(_otptCurVal == _otptValMin){
				_turnOnSldrMin();
				if(_autoSwpDirOnEnd == true){
					_curSldrDirUp = true;
				}
			}
		}
	}

	return;
}

void SldrDALtchMPBttn::stOnStrtScndMod_In(){
	if(!_isOnScndry)
		_turnOnScndry();
	if(_autoSwpDirOnPrss)
		swapSldrDir();

	return;
}

bool SldrDALtchMPBttn::swapSldrDir(){

	return _setSldrDir(!_curSldrDirUp);
}

void SldrDALtchMPBttn::_turnOffSldrMax(){
	/*if(_otptCurValIsMax){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffSldrMax != nullptr){
			_fnWhnTrnOffSldrMax();
		}
		if(_fnVdPtrPrmWhnTrnOffSldrMax != nullptr){
			_fnVdPtrPrmWhnTrnOffSldrMax(_fnVdPtrPrmWhnTrnOffSldrMaxArgPtr);
		}
		//---------------->> Flags related actions
		_otptCurValIsMax = false;
		setOutputsChange(true);
	}*/	//TODO Activate the previous code segment

	_otptCurValIsMax = false;

	return;
}

void SldrDALtchMPBttn::_turnOnSldrMax(){
	/*if(!_otptCurValIsMax){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnSldrMax != nullptr){
			_fnWhnTrnOnSldrMax();
		}
		if(_fnVdPtrPrmWhnTrnOnSldrMax != nullptr){
			_fnVdPtrPrmWhnTrnOnSldrMax(_fnVdPtrPrmWhnTrnOnSldrMaxArgPtr);
		}
		//---------------->> Flags related actions
		_otptCurValIsMax = true;
		setOutputsChange(true);
	}*/	//TODO Activate the previous code segment

	_otptCurValIsMax = true;

	return;
}

void SldrDALtchMPBttn::_turnOffSldrMin(){
	/*if(_otptCurValIsMin){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffSldrMin != nullptr){
			_fnWhnTrnOffSldrMin();
		}
		if(_fnVdPtrPrmWhnTrnOffSldrMin != nullptr){
			_fnVdPtrPrmWhnTrnOffSldrMin(_fnVdPtrPrmWhnTrnOffSldrMinArgPtr);
		}
		//---------------->> Flags related actions
		_otptCurValIsMin = false;
		setOutputsChange(true);
	}*/	//TODO Activate the previous code segment

	_otptCurValIsMin = false;

	return;
}

void SldrDALtchMPBttn::_turnOnSldrMin(){
	/*if(!_otptCurValIsMin){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnSldrMin != nullptr){
			_fnWhnTrnOnSldrMin();
		}
		if(_fnVdPtrPrmWhnTrnOnSldrMin != nullptr){
			_fnVdPtrPrmWhnTrnOnSldrMin(_fnVdPtrPrmWhnTrnOnSldrMinArgPtr);
		}
		//---------------->> Flags related actions
		_otptCurValIsMin = true;
		setOutputsChange(true);
	}*/	//TODO Activate the previous code segment

	_otptCurValIsMin = true;

	return;
}

//=========================================================================> Class methods delimiter

VdblMPBttn::VdblMPBttn()
{
}

VdblMPBttn::VdblMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:DbncdDlydMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay)
{
	_isOnDisabled = isOnDisabled;
}

VdblMPBttn::~VdblMPBttn()
{
}

void VdblMPBttn::clrStatus(bool clrIsOn){
	if(_isVoided){
		setIsNotVoided();
	}
	DbncdMPBttn::clrStatus(clrIsOn);

	return;
}

fncPtrType VdblMPBttn::getFnWhnTrnOffVdd(){

	return _fnWhnTrnOffVdd;
}

fncPtrType VdblMPBttn::getFnWhnTrnOnVdd(){
	
	return _fnWhnTrnOnVdd;
}

bool VdblMPBttn::getFrcOtptLvlWhnVdd(){

	return _frcOtptLvlWhnVdd;
}

fncVdPtrPrmPtrType VdblMPBttn::getFVPPWhnTrnOffVdd(){
	
	return _fnVdPtrPrmWhnTrnOffVdd;
}

void* VdblMPBttn::getFVPPWhnTrnOffVddArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOffVddArgPtr;
}

fncVdPtrPrmPtrType VdblMPBttn::getFVPPWhnTrnOnVdd(){
	
	return _fnVdPtrPrmWhnTrnOnVdd;
}

void* VdblMPBttn::getFVPPWhnTrnOnVddArgPtr(){
	
	return _fnVdPtrPrmWhnTrnOnVddArgPtr;
}

const bool VdblMPBttn::getIsVoided() const{

    return _isVoided;
}

bool VdblMPBttn::getStOnWhnOtpFrcd(){

	return _stOnWhnOtptFrcd;
}

void VdblMPBttn::mpbPollCallback(){
	if(_mpbInstnc->getIsEnabled()){
		// Input/Output signals update
		updIsPressed();
		// Flags/Triggers calculation & update
		updValidPressesStatus();
		updVoidStatus();
	}
 	// State machine state update
	updFdaState();

	return;
}

uint32_t VdblMPBttn::_otptsSttsPkg(uint32_t prevVal){
	prevVal = DbncdMPBttn::_otptsSttsPkg(prevVal);

	if(_isVoided)
		prevVal |= ((uint32_t)1) << IsVoidedBitPos;
	else
		prevVal &= ~(((uint32_t)1) << IsVoidedBitPos);

	return prevVal;
}

void VdblMPBttn::setFnWhnTrnOffVddPtr(void(*newFnWhnTrnOff)()){
	if (_fnWhnTrnOffVdd != newFnWhnTrnOff)
		_fnWhnTrnOffVdd = newFnWhnTrnOff;

	return;
}

void VdblMPBttn::setFnWhnTrnOnVddPtr(void(*newFnWhnTrnOn)()){
	if (_fnWhnTrnOnVdd != newFnWhnTrnOn)
		_fnWhnTrnOnVdd = newFnWhnTrnOn;

	return;
}

void VdblMPBttn::setFrcdOtptWhnVdd(const bool &newVal){
	if(_frcOtptLvlWhnVdd != newVal)
		_frcOtptLvlWhnVdd = newVal;

	return;
}

void VdblMPBttn::setFVPPWhnTrnOffVdd(fncVdPtrPrmPtrType newFVPPWhnTrnOff, void* argPtr){
	if (_fnVdPtrPrmWhnTrnOffVdd != newFVPPWhnTrnOff){
		_fnVdPtrPrmWhnTrnOffVdd = newFVPPWhnTrnOff;
		_fnVdPtrPrmWhnTrnOffVddArgPtr = argPtr;
	}

	return;	
}

void VdblMPBttn::setFVPPWhnTrnOffVddArgPtr(void* newFVPPWhnTrnOffArgPtr){
	if (_fnVdPtrPrmWhnTrnOffVddArgPtr != newFVPPWhnTrnOffArgPtr)
		_fnVdPtrPrmWhnTrnOffVddArgPtr = newFVPPWhnTrnOffArgPtr;

	return;
}

void VdblMPBttn::setFVPPWhnTrnOnVdd(fncVdPtrPrmPtrType newFVPPWhnTrnOn, void* argPtr){
	if (_fnVdPtrPrmWhnTrnOnVdd != newFVPPWhnTrnOn){
		_fnVdPtrPrmWhnTrnOnVdd = newFVPPWhnTrnOn;
		_fnVdPtrPrmWhnTrnOnVddArgPtr = argPtr;
	}

	return;
}

void VdblMPBttn::setFVPPWhnTrnOnVddArgPtr(void* newFVPPWhnTrnOnArgPtr){
	if (_fnVdPtrPrmWhnTrnOnVddArgPtr != newFVPPWhnTrnOnArgPtr)
		_fnVdPtrPrmWhnTrnOnVddArgPtr = newFVPPWhnTrnOnArgPtr;

	return;
}

bool VdblMPBttn::setIsNotVoided(){

	return setVoided(false);
}

bool VdblMPBttn::setIsVoided(){

	return setVoided(true);
}

void VdblMPBttn::setStOnWhnOtpFrcd(const bool &newVal){
	if(_stOnWhnOtptFrcd != newVal)
		_stOnWhnOtptFrcd = newVal;

	return;
}

bool VdblMPBttn::setVoided(const bool &newVoidValue){
	if(_isVoided != newVoidValue){
		if(newVoidValue)
			_turnOnVdd();
		else
			_turnOffVdd();
	}

	return true;
}

void VdblMPBttn::stDisabled_In(){
	if(_isOn != _isOnDisabled){
		if(_isOn)
			_turnOff();
		else
			_turnOn();
	}
	clrStatus(false);	//Clears all flags and timers, _isOn value will not be affected

	return;
}

void VdblMPBttn::stDisabled_Out(){
	clrStatus(true);	//Clears all flags and timers, _isOn value **will** be reset

	return;
}

void VdblMPBttn::_turnOffVdd(){

	if(_isVoided){
		//---------------->> Functions related actions
		if(_fnWhnTrnOffVdd != nullptr)
			_fnWhnTrnOffVdd();
		if(_fnVdPtrPrmWhnTrnOffVdd != nullptr)
			_fnVdPtrPrmWhnTrnOffVdd(_fnVdPtrPrmWhnTrnOffVddArgPtr);
		//---------------->> Flags related actions
		_isVoided = false;
		setOutputsChange(true);
	}

	return;
}

void VdblMPBttn::_turnOnVdd(){

	if(!_isVoided){
		//---------------->> Functions related actions
		if(_fnWhnTrnOnVdd != nullptr)
			_fnWhnTrnOnVdd();
		if(_fnVdPtrPrmWhnTrnOnVdd != nullptr)
			_fnVdPtrPrmWhnTrnOnVdd(_fnVdPtrPrmWhnTrnOnVddArgPtr);
		//---------------->> Flags related actions
		_isVoided = true;
		setOutputsChange(true);
	}

	return;
}

void VdblMPBttn::updFdaState(){
	switch(_mpbFdaState){
		case stOffNotVPP:
			//In: >>---------------------------------->>
			if(_sttChng){
				stOffNotVPP_In();
				_turnOffVdd();		//This should be part of stOffNotVPP_In(), refactoring needed
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validPressPend){
				_mpbFdaState = stOffVPP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
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
			stOffVPP_Do();	// This provides a setting point for the voiding mechanism to be started
			_mpbFdaState = stOnNVRP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnNVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validVoidPend){
				_mpbFdaState = stOnVVP;
				setSttChng();
			}
			if(_validReleasePend){
				_mpbFdaState = stOnVRP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVVP:
			if(_sttChng){
				_turnOnVdd();
				_validVoidPend = false;
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOnVddNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnVddNVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOffVddNVUP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOffVddNVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			stOffVddNVUP_Do();
			if(_validUnvoidPend){
				_mpbFdaState = stOffVddVUP;
				setSttChng();
			}
			if(_validDisablePend){
				_mpbFdaState = stDisabled;	//The MPB has been disabled
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOffVddVUP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOffVdd();
			_validUnvoidPend = false;
			_mpbFdaState = stOffUnVdd;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOffUnVdd:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stOnVRP:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_validReleasePend = false;
			_mpbFdaState = stOnTurnOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOnTurnOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_turnOff();
			_mpbFdaState = stOff;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
//!		break;	// This state makes no conditional next state setting, and it's next state is next in line, let it cascade

		case stOff:
			//In: >>---------------------------------->>
			if(_sttChng){clrSttChng();}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			_mpbFdaState = stOffNotVPP;
			setSttChng();
			//Out: >>---------------------------------->>
			if(_sttChng){}	// Execute this code only ONCE, when exiting this state
			break;

		case stDisabled:
			//In: >>---------------------------------->>
			if(_sttChng){
				_validDisablePend = false;
				stDisabled_In();
				_isEnabled = false;
				setOutputsChange(true);
				clrSttChng();
			}	// Execute this code only ONCE, when entering this state
			//Do: >>---------------------------------->>
			if(_validEnablePend){
				_turnOff();
				_isEnabled = true;
				_validEnablePend = false;
				setOutputsChange(true);
			}
			if(_isEnabled && !updIsPressed()){	//The stDisabled status will be kept until the MPB is released for security reasons
				_mpbFdaState = stOffNotVPP;
				setSttChng();
			}
			//Out: >>---------------------------------->>
			if(_sttChng){
				stDisabled_Out();
			}	// Execute this code only ONCE, when exiting this state
			break;

		default:
			break;
	}

	return;
}

//=========================================================================> Class methods delimiter

TmVdblMPBttn::TmVdblMPBttn()
{
}

TmVdblMPBttn::TmVdblMPBttn(const uint8_t &mpbttnPin, unsigned long int voidTime, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay, const bool &isOnDisabled)
:VdblMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, isOnDisabled), _voidTime{voidTime}
{
}

TmVdblMPBttn::~TmVdblMPBttn()
{
}

bool TmVdblMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};

	result =  DbncdDlydMPBttn::begin(pollDelayMs);

   return result;
}

void TmVdblMPBttn::clrStatus(){
   _voidTmrStrt = 0;
   VdblMPBttn::clrStatus();

   return;
}

const unsigned long int TmVdblMPBttn::getVoidTime() const{

	return _voidTime;
}

bool TmVdblMPBttn::setVoidTime(const unsigned long int &newVoidTime){
	bool result{true};

	if(newVoidTime != _voidTime){
		if(newVoidTime >= _MinSrvcTime)
			_voidTime = newVoidTime;
		else
			result = false;
	}

	return result;
}

void TmVdblMPBttn::stOffNotVPP_In(){
	_voidTmrStrt = 0;

	return;
}

void TmVdblMPBttn::stOffVddNVUP_Do(){
	if(_validReleasePend){
		_validReleasePend = false;
		_validUnvoidPend = true;
	}

	return;
}

void TmVdblMPBttn::stOffVPP_Do(){	// This provides a setting point for the voiding mechanism to be started
   _voidTmrStrt = millis();

	return;
}

bool TmVdblMPBttn::updIsPressed(){

	return DbncdDlydMPBttn::updIsPressed();
}

bool TmVdblMPBttn::updVoidStatus(){
   bool result {false};

   if(_voidTmrStrt != 0){
		if ((millis() - _voidTmrStrt) >= (_voidTime)){ // + _dbncTimeTempSett + _strtDelay
			result = true;
		}
	}
   _validVoidPend = result;

	return _validVoidPend;
}

//=========================================================================> Class methods delimiter

SnglSrvcVdblMPBttn::SnglSrvcVdblMPBttn()
{
}

SnglSrvcVdblMPBttn::SnglSrvcVdblMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp, const bool &typeNO, const unsigned long int &dbncTimeOrigSett, const unsigned long int &strtDelay)
:VdblMPBttn(mpbttnPin, pulledUp, typeNO, dbncTimeOrigSett, strtDelay, false)
{
	_isOnDisabled = false;
   _frcOtptLvlWhnVdd = true;	//This attribute is subclass inherent characteristic, no setter will be provided for it
   _stOnWhnOtptFrcd = false;	//This attribute is subclass inherent characteristic, no setter will be provided for it
}

SnglSrvcVdblMPBttn::~SnglSrvcVdblMPBttn()
{
}

bool SnglSrvcVdblMPBttn::begin(const unsigned long int &pollDelayMs){
   bool result {false};

	result =  DbncdDlydMPBttn::begin(pollDelayMs);

   return result;
}

void SnglSrvcVdblMPBttn::stOffVddNVUP_Do(){
	if(_validReleasePend){
		_validReleasePend = false;
		_validUnvoidPend = true;
	}

	return;
}

bool SnglSrvcVdblMPBttn::updVoidStatus(){
	bool result {false};

	if(_isOn)
		result = true;
	_validVoidPend = result;

	return _validVoidPend;
}

//=========================================================================> Class methods delimiter

unsigned long int findMCD(unsigned long int a, unsigned long int b) {
   unsigned long int result{ 0 };

   if (a != 0 && b != 0) {
		// Use Euclidean algorithm for efficiency
		while (a != b) {
			if (a > b)
				a -= b;
			else 
				b -= a;
		}
		result = a;
	}

	return result; // At this point, a and b are equal and represent the MCD
}

/**
 * @brief Unpackages a 32-bit value into a DbncdMPBttn object status
 * 
 * The 32-bit encoded and packaged is used for inter-task object status comunication, passed as a "notification value" in a xTaskNotify() execution.
 * For each bit value attribute flag represented see DbncdMPBttn::getOtptsSttsPkgd()
 * 
 * @param pkgOtpts A 32-bit value holding a DbncdMPBttn status encoded
 * @return A MpbOtpts_t type element containing the information decoded
 */
/*
+--+--+--+--+--+--+--+--++--+--+--+--+--+--+--+--++--+--+--+--+--+--+--+--++--+--+--+--+--+--+--+--+
|31|30|29|28|27|26|25|24||23|22|21|20|19|18|17|16||15|14|13|12|11|10|09|08||07|06|05|04|03|02|01|00|
 ------------------------------------------------                       ------ -- -- -- -- -- -- --
                                                |                          |    |  |  |  |  |  |  |
                                                |                          |    |  |  |  |  |  |  isOn
                                                |                          |    |  |  |  |  |  isEnabled
                                                |                          |    |  |  |  |  pilotOn
                                                |                          |    |  |  |   wrnngOn
                                                |                          |    |  |  isVoided
                                                |                          |    |  isOnScndry
                                                otptCurVal (16 bits)
*/

 MpbOtpts_t otptsSttsUnpkg(uint32_t pkgOtpts){
	MpbOtpts_t mpbCurSttsDcdd {0};

	if(pkgOtpts & (((uint32_t)1) << IsOnBitPos))
		mpbCurSttsDcdd.isOn = true;
	else
		mpbCurSttsDcdd.isOn = false;

	if(pkgOtpts & (((uint32_t)1) << IsEnabledBitPos))
		mpbCurSttsDcdd.isEnabled = true;
	else
		mpbCurSttsDcdd.isEnabled = false;

	// From here on the attribute flags are not present in every subclass!!
	if(pkgOtpts & (((uint32_t)1) << PilotOnBitPos))
		mpbCurSttsDcdd.pilotOn = true;
	else
		mpbCurSttsDcdd.pilotOn = false;

	if(pkgOtpts & (((uint32_t)1) << WrnngOnBitPos))
		mpbCurSttsDcdd.wrnngOn = true;
	else
		mpbCurSttsDcdd.wrnngOn = false;

	if(pkgOtpts & (((uint32_t)1) << IsVoidedBitPos))
		mpbCurSttsDcdd.isVoided = true;
	else
		mpbCurSttsDcdd.isVoided = false;

	if(pkgOtpts & (((uint32_t)1) << IsOnScndryBitPos))
		mpbCurSttsDcdd.isOnScndry = true;
	else
		mpbCurSttsDcdd.isOnScndry = false;

	mpbCurSttsDcdd.otptCurVal = (pkgOtpts & 0xffff0000) >> OtptCurValBitPos;

	return mpbCurSttsDcdd;
}

