/**
  ******************************************************************************
  * @file	: ButtonToSwitch_AVR.h
  * @brief	: Header file for the ButtonToSwitch_AVR library classes
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
#ifndef _BUTTONTOSWITCH_AVR_H_
#define _BUTTONTOSWITCH_AVR_H_

#include <Arduino.h>
#include <stdint.h>

//===========================>> BEGIN General use constant definitions
#define _HwMinDbncTime 20   //Documented minimum wait time for a MPB signal to stabilize
#define _StdPollDelay 10
#define _MinSrvcTime 100
#define _InvalidPinNum 0xFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment related

/*---------- DbncdMPBttn complete status related constants, argument structs, information packing and unpacking BEGIN -------*/
const uint8_t IsOnBitPos {0};
const uint8_t IsEnabledBitPos{1};
const uint8_t PilotOnBitPos{2};
const uint8_t WrnngOnBitPos{3};
const uint8_t IsVoidedBitPos{4};
const uint8_t IsOnScndryBitPos{5};
const uint8_t OtptCurValBitPos{16};
//===========================>> END General use constant definitions

#ifndef MPBOTPTS_T
	#define MPBOTPTS_T
	/**
	 * @brief Type to hold the complete set of output attribute flags from any DbncdMPBttn class and subclasses object.
	 *
	 * Only two members (isOn and isEnabled) are relevant to all classes, the rest of the members might be relevant for one or more of the DbcndMPBttn subclasses.
	 * The type is provided as a standard return value for the decoding of the 32-bit notification value provided by the use of the xTaskNotify() inter-task mechanism. See setTaskToNotify(const TaskHandle_t) for more information.
	 */
	struct MpbOtpts_t{
		bool isOn;
		bool isEnabled;
		bool pilotOn;
		bool wrnngOn;
		bool isVoided;
		bool isOnScndry;
		uint16_t otptCurVal;
	};
#endif
/*---------------- DbncdMPBttn complete status related constants, argument structs, information packing and unpacking END -------*/

// Definition workaround to let a function/method return value to be a function pointer
typedef void (*fncPtrType)();
typedef  fncPtrType (*ptrToTrnFnc)();

//===========================>> BEGIN General use function prototypes
MpbOtpts_t otptsSttsUnpkg(uint32_t pkgOtpts);
unsigned long int findMCD(unsigned long int a, unsigned long int b);

//===========================>> END General use function prototypes

/**
 * @note This ButtonToSwitch_AVR implementation relies on the TimerOne library by paulstoffregen to manage the time generated INT.  
 * 
 * Being the facilities provided by the TimerOne limited to the execution of only ONE timer interrupt, the setup selected for this development is the following:
 * - A static array of pointers to all the DbncdMPBttn class and subclasses objects -pointed by **_mpbsInstncsLstPtr**- to be kept updated is created automatically when the first element is added with the **begin()** method. This **"list of MPBs to keep updated"** will grow as more elements are added. The object elements will be taken out of the list by using the **end()** method. When the list is left with no elements it'll be deleted automatically.
 * - Each object will hold the attribute for it's time period between updates: **_pollPeriodMs**
 * - Each object will hold the attribute for the last time it was updated: **_lstPollTime**
 * - Each object will hold the attribute flag to be included or ignored for the periodic update -to be used by the pause() and resume() methods- **_updTmrAttchd**. Depending on that attribute flag value the object will or will be not updated albeit being present in the **"list of MPBs to keep updated"**. This is included for temporary states, avoiding the time and resources needed to take out and replace back an object from the update list by using **begin()** or **end()**
 * The begin() and end() will work the inclusion and exclusion of the object in the **"list of MPBs to keep updated"** -pointed to by **_mpbsInstncsLstPtr**- just verifying the object's **_updTmrAttchd** is set to true when the begin is executed.
 * - The timer interrupt period is a common attribute (static) to all the DbncdMPBttn class and subclasses objects **_updTimerPeriod**, it will be set as the MCD of each and every **timer attached** object in the **"list of MPBs to keep updated"**. The use of a MCD calculated time period is resource optimization oriented, to reduce interrupts to the minimum strictly needed and avoiding a fixed time setting. The best use of the resources by using the longer periods still suitable to do the updating job and selecting different MPBs update time that have a higher MCD is left to the developer best knowledge.
 * That implies that the **_updTimerPeriod** should be updated:
 * - With every begin() invocation.
 * - With every resume() invocation.
 * - With every end() invocation, taking care of the special case if the **"list of MPBs to keep updated"** is emptied (**_updTimerPeriod** = 0 and Timer1 stopped)
 * - With every pause() invocation, taking care of the special case if the **"list of MPBs to keep updated"** is emptied (**_updTimerPeriod** = 0 and Timer1 paused)
 * 
 *  The callback function duties:
 * - Verify for a valid **_mpbsInstncsLstPtr**, if it's nullptr something failed, correct it by disabling the timer
 * - Save the **current time** for reference and traverse the list till the end (nullptr).
 * - For each MPB in the list verify the _updTmrAttchd == true
 * - If _updTmrAttchd == true -> verify the ("current time" - _lstPollTime) >= _pollPeriodMs. If the condition is true:
 * 	- Execute the objects mpbPollCallback()
 * 	- Set _lstPollTime = "current time"
 */

//==========================================================>> Classes declarations BEGIN

/**
 * @brief Base class, models a Debounced Momentary Push Button (**D-MPB**).
 *
 * This class provides the resources needed to process a momentary digital input signal -as the one provided by a MPB (Momentary Push Button)- returning a clean signal to be used as a switch, implementing the needed services to replace a wide range of physical related switch characteristics: Debouncing, deglitching, disabling.
 *
 * More physical switch situations can be emulated, like temporarily disconnecting it (isDisabled=true and isOnDisabled=false), short circuiting it (isDisabled=true and isOnDisabled=true) and others.
 *
 * @class DbncdMPBttn
 */
class DbncdMPBttn{
	static DbncdMPBttn** _mpbsInstncsLstPtr;
	static unsigned long int _updTimerPeriod;
/*
 * This is the callback function to be executed by the TimerOne managed timer INT.
 * 
 * The callback function duties:
 * - Verify for a valid **_mpbsInstncsLstPtr**, if it's nullptr something failed, correct it by disabling the timer
 * - Save the **current time** for reference and traverse the list till the end (nullptr).
 * - For each MPB in the list verify the _updTmrAttchd == true
 * - If _updTmrAttchd == true -> verify the ("current time" - _lstPollTime) >= _pollPeriodMs. If the condition is true:
 * 	- Execute the objects mpbPollCallback()
 * 	- Set _lstPollTime = "current time" * 
 */
	static void _ISRMpbsRfrshCb();

protected:
	enum fdaDmpbStts {
		stOffNotVPP,
		stOffVPP,
		stOn,
		stOnVRP,
		stDisabled
	};
	const unsigned long int _stdMinDbncTime {_HwMinDbncTime};

	uint8_t _mpbttnPin{};
	bool _pulledUp{};
	bool _typeNO{};
	unsigned long int _dbncTimeOrigSett{};

	unsigned long int _dbncRlsTimerStrt{0};
	unsigned long int _dbncRlsTimeTempSett{0};
	unsigned long int _dbncTimerStrt{0};
	unsigned long int _dbncTimeTempSett{0};
	void (*_fnWhnTrnOff)() {nullptr};
	void (*_fnWhnTrnOn)() {nullptr};
	bool _isEnabled{true};
	volatile bool _isOn{false};
	bool _isOnDisabled{false};
	volatile bool _isPressed{false};
	unsigned long int _lstPollTime{0};
	fdaDmpbStts _mpbFdaState {stOffNotVPP};
	DbncdMPBttn* _mpbInstnc{nullptr};
	volatile bool _outputsChange {false};
	uint32_t _outputsChangeCnt{0};
	unsigned long int _pollPeriodMs{0};
	bool _prssRlsCcl{false};
	unsigned long int _strtDelay {0};
	bool _sttChng {true};
	bool _updTmrAttchd{false};
	volatile bool _validDisablePend{false};
	volatile bool _validEnablePend{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};

   void clrSttChng();
	const bool getIsPressed() const;
	virtual void mpbPollCallback();
	virtual uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
	void _popMpb(DbncdMPBttn** &DMpbTmrUpdLst, DbncdMPBttn* mpbToPop);
	void _pushMpb(DbncdMPBttn** &DMpbTmrUpdLst, DbncdMPBttn* mpbToPush);
	void _setIsEnabled(const bool &newEnabledValue);
	void _setLstPollTime(const unsigned long int &newLstPollTIme);	//! Implemented to avoid direct manipulation of the _lstPollTime attribute
	void setSttChng();
	void _turnOff();
	void _turnOn();
	virtual void updFdaState();
	bool updIsPressed();
	unsigned long int _updTmrsMCDCalc();
	virtual bool updValidPressesStatus();

public:    
	/**
	 * @brief Default class constructor
	 *
	 */
	DbncdMPBttn();
	/**
	 * @brief Class constructor
	 *
	 * @param mpbttnPin Pin id number of the input signal pin
	 * @param pulledUp (Optional) boolean, indicates if the input pin must be configured as INPUT_PULLUP (true, default value), or INPUT_PULLDOWN (false), to calculate correctly the expected voltage level in the input pin. The pin is configured by the constructor so no previous programming is needed. The pin must be free to be used as a digital input, and must be a pin with an internal pull-up circuit, as not every GPIO pin has the option.
	 * @param typeNO (Optional) boolean, indicates if the MPB is a Normally Open (NO) switch (true, default value), or Normally Closed (NC) (false), to calculate correctly the expected level of voltage indicating the MPB is pressed.
	 * @param dbncTimeOrigSett (Optional) unsigned long integer (uLong), indicates the time (in milliseconds) to wait for a stable input signal before considering the MPB to be pressed (or not pressed). If no value is passed the constructor will assign the minimum value provided in the class, that is 20 milliseconds as it is an empirical value obtained in various published tests.
	 *
	 */
	DbncdMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
 * @brief Default virtual destructor
 *
 */
	virtual ~DbncdMPBttn();
	/**
	 * @brief Attaches the instantiated object to a timer that monitors the input pins and updates the object status.
	 * 
	 * The frequency of the periodic monitoring is passed as a parameter in milliseconds, and is a value that must be small (frequent) enough to keep the object updated, but not so frequent that wastes resources from other tasks. A default value is provided based on empirical results obtained in various published tests.
	 * 
	 * @attention Due to the fact that the available resources limits the timers available to a single one, attaching the timer to keep different instantiated objects status updated involves several steps:
	 * - The method adds the object to an array of MPBttns objects to keep updated (after checking the object was not already included in the array).
	 * - As every object in the array has an independent time setting to be updated, a calculus must be made to set the timer to the best suited time to reduce the number of interrupts of the normal execution of the main code to check if any MPBttn object is set to be updated, while keeping those status updated in the intended time.
	 * - If this is the first object to be added to the status update array (or all the objects in the array were in **Paused State**, so the timer interrupt was disabled), set the timer period and **start the timer**.
	 * - If this is not the first active (not paused) object in the status update array **modify (if required) the timer set period** to the new calculated one
	 *
	 * @param pollDelayMs (Optional) unsigned long integer (ulong), the time between polls in milliseconds.
	 *
	 * @return Boolean indicating if the object could be attached to a timer.
	 * @retval true: the object could be attached to the timer -or it was already attached to a timer when the method was invoked-.
	 * @retval false: the object could not be attached to the timer, because the parameter passed for the timer was 0 (zero).
	 */
	virtual bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief Clears and resets flags, timers and counters modified through the object's signals processing.
	 *
	 * Resets object's attributes to its initialization values to safely resume operations, either after pausing the timer, enabling the object after disabling it or any disruptive activity that might generate unexpected distorsions. This avoids risky behavior of the object due to dangling flags or partially consumed time counters.
	 *
	 * @param clrIsOn Optional boolean value, indicates if the _isOn flag must be included to be cleared:
	 *
	 * - true (default value) includes the isOn flag.
	 * - false excludes the isOn flag.
	 */
	void clrStatus(bool clrIsOn = true);
   /**
	 * @brief Disables the input signal processing, ignoring the changes of its values.
	 *
	 * Invoking the disable() method sends a message to the object requesting it to enter the **Disabled state**. Due to security and stability of the object's behavior the message will not be processed at every stage of the input signals detection and output computations, but in every safe and stable stage. When the message is processed the object will:
	 * - Stop all input signal reading.
	 * - Stop any new output flag computation.
	 * - Reset all output flag values.
	 * - Force the isOn flag according to the **isOnDisabled** flag setting.
	 * - Keep this **Disabled state** behavior until an enabling message is received through a **enable()** method.
    */
	void disable();
   /**
	 * @brief Enables the input signal processing.
	 *
	 * Invoking the enable() method on a object in **Disabled state** sends it a message requesting to resume it's normal operation. The execution of the re-enabling of the object implies:
	 * - Resuming all input signals reading.
	 * - Resuming all output flag computation from the "fresh startup" state, including clearing the **isOn state**
	 * - Due to strict security enforcement the object will not be allowed to enter the **Enabled state** if the MPB was pressed when the enable message was received and until a MPB release is efectively detected.
    */
	void enable();
	/**
	 * @brief Detaches the object from the timer that monitors the input pins, compute and updates the object's status.
	 *
	 * @attention Due to the fact that the available resources limits the timers available to a single one (see bool begin(const unsigned long int) for details ), dettaching the object from the timer involves several steps:
	 * - Pausing the object: that will take care of the recalculation of the the update time period, setting it and stop the timer if no active objects are left in the list.
	 * - Removing the object from the list of objects to keep updated.
	 * 
	 * @note The immediate detachment of the object from the timer that keeps it's state updated implies that the object's state will be kept, whatever that state is it. If a certain status is preferred some of the provided methods should be used for that including clrStatus(), resetFda(), disable(), setIsOnDisabled(), etc.
	 *
	 * @return Boolean indicating the success of the operation
	 * @retval true: the object detachment procedure and timer entry removal was successful.
	 * @retval false: the object detachment and/or entry removal was rejected by the O.S..
	 */
	bool end();    
	/**
	 * @brief Returns the current debounce period time set for the object.
	 *
	 * The original value for the debouncing process used at instantiation time might be changed with the **setDbncTime(const unsigned long int)** or with the **resetDbncTime()** methods. This method gets the current value of the attribute in use.
	 *
	 * @return The current debounce time in milliseconds
	 */
	const unsigned long int getCurDbncTime() const;
	/**
	 * @brief Returns the function that is set to execute every time the object **enters** the **Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **Off State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **Off State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it entering the **Off State**, including the modification of affected attribute flags, suspending the execution of the task running while in **On State** and others. Making the function code too time demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOff();
	/**
	 * @brief Returns the function that is set to execute every time the object **enters** the **On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object enters the **On State**.
	 * @retval nullptr if there is no function set to execute when the object enters the **On State**.
	 *
	 * 	 * @warning The function code execution will become part of the list of procedures the object executes when it entering the **On State**, including the modification of affected attribute flags, suspending the execution of the task running while in **On State** and others. Making the function code too time demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOn();
   /**
	 * @brief Returns the value of the isEnabled attribute flag, indicating the **Enabled** or **Disabled** status of the object.
	 *
	 * The isEnabled flag might be modified by the enable() and the disable() methods.
	 *
    * @retval true: the object is in **Enabled state**.
    * @retval false: The object is in **Disabled state**.
    */
	const bool getIsEnabled() const;
   /**
	 * @brief Returns the value of the **isOn** attribute flag.
	 *
	 * The **isOn** attribute flag is the fundamental attribute of the object, it might be considered the "Raison d'etre" of all this classes design: the isOn signal is not just the detection of an expected voltage value at a mcu pin, but the combination of that voltage level, filtered and verified, for a determined period of time and until a new event modifies that situation.  While other mechanism are provided to execute code when the status of the object changes, all but the **isOn** flag value update are optionally executed.
	 *
    * @retval true: The object is in **On state**.
    * @retval false: The object is in **Off state**.
    */
	const bool getIsOn () const;
   /**
	 * @brief Returns the value of the **isOnDisabled** attribute flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed when needed.
	 * In the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this attribute flag's value.
	 * The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent.
	 * The flag value might be changed by the use of the **setIsOnDisabled()** method.
    *
    * @retval true: the object is configured to be set to the **On state** while it is in **Disabled state**.
    * @retval false: the object is configured to be set to the **Off state** while it is in **Disabled state**.
    */
	const bool getIsOnDisabled() const;
	/**
	 * @brief Returns the time stamp of the last state update for the object.
	 * 
	 * The use of a unique timer to keep updated the state of all MPB objects forces each object to keep track of the time passed since it's last update. This is due to the fact that each MPB can be configured to have it's status updated at it's own rate. Keeping a register of the last update gives each MPB the possibility to compute at each Timer interruption signaling if it's the time to get the state update done.
	 * 
	 * @return An unsigned long int representing the time stamp in milliseconds for the last time the state update was executed for the current object.
	 */
	const unsigned long int getLstPollTime();
   /**
    * @brief Returns the relevant attribute flags values for the object state encoded as a 32 bits value, useful to pass current state of the object to a function managing the outputs as a parameter.
    *
    * @return A 32-bit unsigned value representing the object's attribute flags current values.
    */
	const uint32_t getOtptsSttsPkgd();
   /**
	 * @brief Returns the value of the **outputsChange** attribute flag.
	 *
	 * The instantiated objects include attributes linked to their computed state, which represent the behavior expected from their respective electromechanical simulated counterparts.
	 * When any of those attributes values change, the **outputsChange** flag is set. The flag only signals changes have happened -not which flags, nor how many times changes have taken place- since the last **outputsChange** flag reset.
	 * The **outputsChange** flag must be reset (or set if desired) through the setOutputsChange() method.
	 *
    * @retval true: any of the object's behavior flags have changed value since last time **outputsChange** flag was reseted.
    * @retval false: no object's behavior flags have changed value since last time **outputsChange** flag was reseted.
	 */
	const bool getOutputsChange() const;
	/**
	 * @brief Returns the poll period time setting attribute's value
	 * 
	 * The poll period time in milliseconds (pollPeriodMs) attribute sets the time period between state updates for the current object. The time period, in milliseconds, is set when the object begin(&pollParam) method is called, and will be conserved through the use of the pause() and resume() methods. The only way to change it is by calling the end() method and then restarting the timer polling with a new begin() with a different value as parameter.
	 * 
	 * @attention The value passed in the begin() method is very important for the object and the system behavior. Setting a very small value will be resources consuming, as the timer will be interrupting the normal execution to keep the object state updated, surely more frequently than really needed. Setting a very large value will result in a very slow updated object, making the execution less responsive than needed. See virtual bool begin(const unsigned long int) for more details. If not provided in the begin() method a standard value of 10 milliseconds is used.
	 * 
	 * @return The time setting for the poll period time in milliseconds.
	 */
	const unsigned long int getPollPeriodMs();
   /**
    * @brief Returns the current value of strtDelay attribute.
    *
    * Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the strtDelay time, no press will be detected by the object, and the isOn flag will not be affected. The original value for the delay process used at instantiation time might be changed with the **setStrtDelay()** method, so this method is provided to get the current value in use.
    *
    * @return The current strtDelay time in milliseconds.
    *
    * @attention The strtDelay attribute is forced to a 0 ms value at instantiation of DbncdMPBttn class objects, and no setter mechanism is provided in this class. The inherited DbncdDlydMPBttn class objects (and all it's subclasses) constructor includes a parameter to initialize the strtDelay value, and a method to set that attribute to a new value. This implementation is needed to keep backwards compatibility to old versions of the library.
    */
	unsigned long int getStrtDelay();
	/**
	 * @brief Returns the value of the Attached to the update timer attribute
	 * 
	 * Even when the instantiated object might be included in  the **"list of MPBs to keep updated"**, the final element that defines if an object is intended to have it's status updated is the attribute "Attached to the updating timer" **updTmrAttchd**. This flag is used to include or exclude the object from the updating process, without having to eliminate it or include it in the mentioned array list, specially useful for situations like the pause() and resume() methods.
	 * 
	 * @return The value of the updTmrAttchd attribute value
	 * @retval true: The object is set up to be updated by the timer events
	 * @retval false: The object is set up NOT to be updated by the timer events
	 */
	bool getUpdTmrAttchd();
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(const uint8_t, const bool, const bool, const unsigned long int)
	 */
	bool init(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Pauses the software timer updating the computation of the object's internal flags value (object's state).
	 *
	 * The immediate stop of the timer that keeps the object's state updated implies that the object's state will be kept, whatever that state is it. The same consideration as the end() method applies referring to options to modify the state in which the object will be while in the **Pause state**.
	 *
	 * @retval true: the object's timer could be stopped by the O.S.(or it was already stopped).
	 * @retval false: the object's timer couldn't be stopped by the O.S..
	 */
	bool pause();
	/**
	 * @brief Resets the debounce process time of the object to the value used at instantiation.
	 *
	 *  The debounce process time used at instantiation might be changed with the setDbncTime() as needed, as many times as needed. This method reverts the value to the instantiation time value. In case the value was not specified at instantiation time the default debounce time value will be used.
	 */
	void resetDbncTime();
	/**
	 * @brief Resets the MPB behavior automaton to it's **Initial** or **Start State**
	 *
	 * This method is provided for security and for error handling purposes, so that in case of unexpected situations detected, the driving **Deterministic Finite Automaton** used to compute the MPB objects states might be reset to it's initial state to safely restart it, maybe as part of an **Error Handling** procedure.
	 */
	void resetFda();
	/**
	 * @brief Restarts the software timer updating the calculation of the object internal flags.
	 *
	 *  The timer will stop calling the functions for computing the flags values after calling the **pause()** method and will not be updated until the timer is restarted with this method.
	 *
	 * @retval true: the object's timer could be restarted by the O.S..
	 * @retval false: the object's timer couldn't be restarted by the O.S..
	 *
	 * @warning This method will restart the inactive timer after a **pause()** method. If the object's timer was modified by an **end()* method then a **begin()** method will be needed to restart it's timer.
	 */
	bool resume();
	/**
	 * @brief Sets the debounce process time.
	 *
	 *	Sets a new time for the debouncing period. The value must be equal or greater than the minimum empirical value set as a property for all the classes, that value is defined in the _HwMinDbncTime constant (20 milliseconds). A long debounce time will produce a delay in the press event generation, making it less "responsive".
	 *
	 * @param newDbncTime unsigned long integer, the new debounce value for the object.
	 *
	 * @return	A boolean indicating if the debounce time setting was successful.
	 * @retval true: the new value is in the accepted range, the attribute value is updated.
	 * @retval false: the value was out of the accepted range, no change was made.
	 */
	bool setDbncTime(const unsigned long int &newDbncTime);
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **Off State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object **enters** the **Off State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOffPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object **enters** the **On State**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object **enters** the **On State**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOnPtr(void (*newFnWhnTrnOn)());
   /**
	 * @brief Sets the value of the **isOnDisabled** attribute flag.
	 *
	 * When instantiated the class, the object is created in **Enabled state**. That might be changed as needed.
	 * While in the **Disabled state** the input signals for the MPB are not processed, and the output will be set to the **On state** or the **Off state** depending on this flag value.
	 *
	 * @note The reasons to disable the ability to change the output, and keep it on either state until re-enabled are design and use dependent, being an obvious one security reasons, disabling the ability of the users to manipulate the switch while keeping the desired **On/Off state**. A simple example would be to keep a light on in a place where a meeting is taking place, disabling the lights switches and keeping the **On State**. Another obvious one would be to keep a machine off while servicing it's internal mechanisms, disabling the possibility of turning it on.
    *
    * @warning If the method is invoked while the object is disabled, and the **isOnDisabled** attribute flag is changed, then the **isOn** attribute flag will have to change accordingly. Changing the **isOn** flag value implies that **all** the implemented mechanisms related to the change of the **isOn** attribute flag value will be invoked.
    */
	void setIsOnDisabled(const bool &newIsOnDisabled);
   /**
	 * @brief Sets the value of the attribute flag indicating if a change took place in any of the output attribute flags (IsOn included).
	 *
	 * The usual path for the **outputsChange** flag is to be set by any method changing an output attribute flag, the callback function signaled to take care of the hardware actions because of this changes clears back **outputsChange** after taking care of them. In the unusual case the developer wants to "intercept" this sequence, this method is provided to set (true) or clear (false) outputsChange value.
    *
    * @param newOutputChange The new value to set the **outputsChange** flag to.
    */
	void setOutputsChange(bool newOutputsChange);
};

//==========================================================>>

/**
 * @brief Models a Debounced Delayed MPB (**DD-MPB**).
 *
 * The **Debounced Delayed Momentary Button**, keeps the ON state since the moment the signal is stable (debouncing process), plus a delay added, and until the moment the push button is released. The reasons to add the delay are design related and are usually used to avoid registering unintentional presses, or to give some equipment (load) that needs time between repeated activations the benefit of the pause. If the push button is released before the debounce and delay times configured are reached, no press is registered at all. The delay time in this class as in the other that implement it, might be zero (0), defined by the developer and/or modified in runtime.
 *
 * @note If the **delay** attribute is set to 0, the resulting object of this class is equivalent in functionality to a **DbncdMPBttn** class object. The main difference is that the "Start Delay" attribute (strtDelay) will be available for changing at runtime.
 *
 * @class DbncdDlydMPBttn
 */
class DbncdDlydMPBttn: public DbncdMPBttn{
public:
    /**
     * @brief Default constructor
     *
     */
	DbncdDlydMPBttn();
    /**
     * @brief Class constructor
     *
     * @param strtDelay Sets the initial value for the **strtDelay** attribute.
     *
     * @note For the rest of the parameters see DbncdMPBttn(const uint8_t, const bool, const bool, const unsigned long int)
     *
     * @note If the **delay** attribute is set to 0, the resulting object is equivalent in functionality to a **DbncdMPBttn** class object.
     */
	DbncdDlydMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     *
     * @brief see DbncdMPBttn::init(const uint8_t, const bool, const bool, const unsigned long int)
     * 
     * @param strtDelay Sets the initial value for the **strtDelay** attribute.
     *
     * @note For the rest of the parameters see DbncdMPBttn::init(const uint8_t, const bool, const bool, const unsigned long int)
     */
	bool init(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief Sets a new value to the "Start Delay" **strtDelay** attribute
     *
     * @param newStrtDelay New value for the "Start Delay" attribute in milliseconds.
     *
     * @note Setting the delay attribute to 0 makes the instantiated object act exactly as a Debounced MPB (D-MPB)
     * 
     * @warning: Using very high **strtDelay** values is valid but might make the system seem less responsive, be aware of how it will affect the user experience.
     */
	void setStrtDelay(const unsigned long int &newStrtDelay);
};

//==========================================================>>

/**
 * @brief Abstract class, base to model Latched Debounced Delayed MPBs (**LDD-MPB**).
 *
 * **Latched DD-MPBs** are MPBs whose distinctive characteristic is that implement switches that keep the ON state since the moment the input signal is stable (debouncing + Delay process), and keeps the ON state after the MPB is released and until an event un-latches them, setting them free to move to the **Off State**.
 * The un-latching mechanisms include but are not limited to: same MPB presses, timers, other MPB presses, other GPIO external un-latch signals or the use of the public method unlatch().
 * The different un-latching events defines the sub-classes of the LDD-MPB class.
 *
 * @attention The range of signals accepted by the instantiated objects to execute the unlatch process is diverse, and their nature and characteristics might affect the expected switch behavior. While some of the signals might be instantaneous, meaning that the **start of the unlatch signal** is coincidental with the **end of the unlatch signal**, some others might extend the time between both ends. To accommodate the logic required by each subclass, and the requirements of each design, the **_unlatch_** process is then split into two stages:
 * 1. Validated Unlatch signal (or Validated Unlatch signal start).
 * 2. Validated Unlatch Release signal (or Validated Unlatch signal end).
 * The class provides methods to generate those validated signals independently of the designated signal source to modify the instantiated object behavior if needed by the design requirements, Validated Unlatch signal (see LtchMPBttn::setUnlatchPend(const bool), Validated Unlatch Release signal (see LtchMPBttn::setUnlatchRlsPend(const bool), or to **set** both flags to generate an unlatch (see LtchMPBttn::unlatch().
 *
 * @warning Generating the unlatch related flags independently of the unlatch signals provided by the LDD-MPB subclasses might result in unexpected behavior, which might generate the locking of the object with it's unexpected consequences.
 *
 * @class LtchMPBttn
 */
class LtchMPBttn: public DbncdDlydMPBttn{
protected:
    enum fdaLmpbStts {
        stOffNotVPP,
        stOffVPP,
        stOnNVRP,
        stOnVRP,
        stLtchNVUP,
        stLtchdVUP,
        stOffVUP,
        stOffNVURP,
        stOffVURP,
        stDisabled
	};
	bool _isLatched{false};
	fdaLmpbStts _mpbFdaState {stOffNotVPP};
	bool _trnOffASAP{true};
	volatile bool _validUnlatchPend{false};
	volatile bool _validUnlatchRlsPend{false};

	virtual void mpbPollCallback();
	virtual void stDisabled_In(){};
	virtual void stDisabled_Out(){};
	virtual void stLtchNVUP_Do(){};
	virtual void stOffNotVPP_In(){};
	virtual void stOffNotVPP_Out(){};
	virtual void stOffNVURP_Do(){};
	virtual void stOffVPP_Out(){};
	virtual void stOffVURP_Out(){};
	virtual void stOnNVRP_Do(){};
	virtual void updFdaState();
	virtual void updValidUnlatchStatus() = 0;
public:
    /**
    * @brief Class constructor
    *
    * @note For the parameters see DbncdDlydMPBttn(const uint8_t, const bool, const bool, const unsigned long int, const unsigned long int)
    */
	LtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
   /**
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
    */
	virtual bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
	/**
	 * @brief See DbncdMPBttn::clrStatus(bool)
	 */
	void clrStatus(bool clrIsOn = true);
   /**
    * @brief Returns the value of the isLatched attribute flag, indicating the **Latched** or **Unlatched** condition of the object.
    *
    * The isLatched flag is automatically updated periodically by the timer that calculates the object state.
	*
    * @retval true: the object is in **Latched condition**.
    * @retval false: The object is in **Unlatched condition**.
    */
	const bool getIsLatched() const;
	/**
	 * @brief Returns the value of the trnOffASAP attribute flag.
	 *
	 * As described in the class characteristics the unlatching process comprises two stages, Validated Unlatch Signal and Validates unlatch Release Signal, that might be generated simultaneously or separated in time. The **trnOffASAP** attribute flag sets the behavior of the MPB in the second case.
	 * - If the **trnOffASAP** attribute flag is set (true) the **isOn** flag will be reset as soon as the **Validated Unlatch Signal** is detected
	 * - If the **trnOffASAP** flag is reset (false) the **isOn** flag will be reset only when the **Validated Unlatch Release signal** is detected.
	 *
	 * @return The current value of the trnOffASAP attribute flag.
	 */
	bool getTrnOffASAP();
	/**
	 * @brief Returns the value of the "Valid Unlatch Pending" attribute
	 *
	 * The "Valid Unlatch Pending" holds the existence of a still to be processed confirmed unlatch signal. Getting it's current value makes possible taking actions before the unlatch process is started or even discard it completely by using the setUnlatchPend(const bool) method.
	 *
	 * @return The current value of the "Valid Unlatch Pending" attribute.
	 */
	const bool getUnlatchPend() const;
	/**
	 * @brief Returns the value of the "Valid Unlatch Release Pending" attribute
	 *
	 * The "Valid Unlatch Release Pending" holds the existence of a still to be processed confirmed unlatch released signal. Getting it's current value makes possible taking actions before the unlatch process ends or even discard it completely by using the setUnlatchRlsPend(const bool) method.
	 *
	 * @return The current value of the "Valid Unlatch Release Pending" attribute.
	 */
	const bool getUnlatchRlsPend() const;
	/**
	 * @brief Sets the value of the trnOffASAP attribute.
	 *
     * As explained in the class description, to accommodate to different sources of the unlatch signal, the unlatching process has been splitted in two steps:
     * 1. Validated Unlatch signal (or Validated Unlatch signal start).
     * 2. Validated Unlatch Release signal (or Validated Unlatch signal end).
     * If trnOffASAP=true, the isOn attribute flag will be reset at the "Validated Unlatch Signal Start" stage.
     * If trnOffASAP=false, the isOn attribute flag will be reset at the "Validated Unlatch Signal End" stage.
     * 
	 * @param newVal New value for the trnOffASAP attribute
	 */
	void setTrnOffASAP(const bool &newVal);
	/**
	 * @brief Sets the value of the "Valid Unlatch Pending" attribute
	 *
	 * By setting the value of the "Valid Unlatch Pending" it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the "Valid Unlatch Pending" attribute
	 */
	void setUnlatchPend(const bool &newVal);
	/**
	 * @brief Sets the value of the "Valid Unlatch Release Pending" attribute
	 *
	 * By setting the value of the "Valid Unlatch Pending" and "Valid Unlatch Release Pending" flags it's possible to modify the current MPB status by generating an unlatch signal or by canceling an existent unlatch signal.
	 *
	 * @param newVal New value for the "Valid Unlatch Release Pending" attribute
	 */
	void setUnlatchRlsPend(const bool &newVal);
	/**
	 * @brief Sets the values of the flags needed to unlatch a latched MPB
	 *
	 * By setting the values of the validUnlatchPending **and** validUnlatchReleasePending flags it's possible to modify the current MPB status by generating an unlatch signal.
	 *
	 * @retval true the object was latched and the unlatch flags were set.
	 * @retval false the object was not latched, no unlatch flags were set.
	 *
	 * @note Setting the values of the validUnlatchPending and validUnlatchReleasePending flags does not implicate immediate unlatching the MPB but providing the unlatching signals. The unlatching signals will be processed by the MPB according to it's embedded behavioral pattern. For example, the signals will be processed if the MPB is in Enabled state and latched, but will be ignored if the MPB is disabled.
	 */
	bool unlatch();
};

//==========================================================>>

/**
 * @brief Models a Toggle Latch DD-MPB, a.k.a. a Toggle Switch (**ToLDD-MPB**).
 *
 * The **Toggle switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and keeps the ON state after the push button is released and until it is pressed once again. So this simulates a simple On-Off switch like the one used to turn on/off a room light or any electric appliance. The included methods lets the designer define the unlatch event as the instant the MPB is started to be pressed for the second time or when the MPB is released from that second press.
 *
 * @class TgglLtchMPBttn
 */
class TgglLtchMPBttn: public LtchMPBttn{
protected:
	virtual void stOffNVURP_Do();
	virtual void updValidUnlatchStatus();
public:
/**
 * @brief Class constructor
 *
 * For the parameters see DbncdMPBttn(const uint8_t, const bool, const bool, const unsigned long int)
 */
	TgglLtchMPBttn(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
};

//==========================================================>>

/**
 * @brief Models a Timer Latch DD-MPB, a.k.a. Timer Switch (**TiLDD-MPB**).
 *
 * The **Timer switch** keeps the **On state** since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preset timer **started immediately after** the MPB has passed the debounce & delay process.
 * The time count down might be reset by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * The total count-down time might be changed by using a provided method.
 *
 * class TmLtchMPBttn
 */
class TmLtchMPBttn: public LtchMPBttn{
protected:
    bool _tmRstbl {true};
    unsigned long int _srvcTime {};
    unsigned long int _srvcTimerStrt{0};

    virtual void stOffNotVPP_Out();
    virtual void stOffVPP_Out();
    virtual void updValidUnlatchStatus();
public:
 	/**
 	 * @brief Class constructor
 	 *
 	 * @param srvcTime The service time (time to keep the **isOn** attribute flag raised).
 	 *
 	 * @note For the other parameters see DbncdDlydMPBttn(const uint8_t, const bool, const bool, const unsigned long int, const unsigned long int)
     */
    TmLtchMPBttn(const uint8_t &mpbttnPin, const unsigned long int &svcTime, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
    /**
     * @brief see DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus(bool clrIsOn = true);
    /**
     * @brief Returns the configured Service Time.
     *
     * @return The current Service Time setting in milliseconds
     */
    const unsigned long int getSrvcTime() const;
    /**
     * @brief Sets a new value to the Service Time attribute
     *
     * @param newSrvcTime New value for the Service Time attribute
     *
     * @note To ensure a safe and predictable behavior from the instantiated objects a minimum Service Time setting guard is provided, ensuring data and signals processing are completed before unlatching process is enforced by the timer. The guard is set by the defined _MinSrvcTime constant.
     *
     * @retval true if the newSrvcTime parameter is equal to or greater than the minimum setting guard, the new value is set.
     * @retval false The newSrvcTime parameter is less than the minimum setting guard, the srvcTime attribute was not changed.
     */
    bool setSrvcTime(const unsigned long int &newSrvcTime);
    /**
     * @brief Configures the timer for the Service Time to be reseted before it reaches unlatching time.
     *
     * If the isResetable attribute flag is cleared the MPB will return to **Off state** when the Service Time is reached no matter if the MPB was pressed again during the service time period. If the attribute flag is set, pressing the MPB (debounce and delay times enforced) while on the **On state** resets the timer, starting back from 0. The reseting might be repeated as many times as desired.
     *
     * @param newIsRstbl The new setting for the isResetable flag.
     */
    void setTmerRstbl(const bool &newIsRstbl);
};

//==========================================================>>

/**
 * @brief Models a Hinted Timer Latch DD-MPB, a.k.a. Staircase Switch (**HTiLDD-MPB**).
 *
 * The **Staircase switch** keeps the ON state since the moment the signal is stable (debouncing + delay process), and until the unlatch signal is provided by a preset timer **started immediately after** the MPB has passed the debounce & delay process.
 * A warning flag might be configured to raise when the time left to keep the ON signal is close to expiration, based on a configurable percentage of the total Service Time.
 * The time count down might be reseted by pressing the MPB before the timer expires by optionally configuring the object to do so with the provided method.
 * A **Pilot Signal** attribute flag is included to emulate completely the staircase switches, that might be activated while the MPB is in **Off state**,  by optionally configuring the object to do so with the provided method (This might be considered just a perk as it's not much more than the **isOn** flag negated output, but gives the advantage of freeing the designer of additional coding).
 *
 * class HntdTmLtchMPBttn
 */
class HntdTmLtchMPBttn: public TmLtchMPBttn{
protected:
	void (*_fnWhnTrnOffPilot)() {nullptr};
	void (*_fnWhnTrnOffWrnng)() {nullptr};
	void (*_fnWhnTrnOnPilot)() {nullptr};
	void (*_fnWhnTrnOnWrnng)() {nullptr};
	bool _keepPilot{false};
	volatile bool _pilotOn{false};
	unsigned long int _wrnngMs{0};
	volatile bool _wrnngOn {false};
	unsigned int _wrnngPrctg {0};

	bool _validWrnngSetPend{false};
	bool _validWrnngResetPend{false};
	bool _validPilotSetPend{false};
	bool _validPilotResetPend{false};

	virtual void mpbPollCallback();
	uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
	virtual void stDisabled_In();
	virtual void stLtchNVUP_Do();
	virtual void stOffNotVPP_In();
	virtual void stOffVPP_Out();
	virtual void stOnNVRP_Do();
	void _turnOffPilot();
	void _turnOffWrnng();
	void _turnOnPilot();
	void _turnOnWrnng();
	bool updPilotOn();
	bool updWrnngOn();
public:
	/**
	 * @brief Class constructor
	 *
	 * @param wrnngPrctg Time **before expiration** of service time that the warning flag must be set. The time is expressed as a percentage of the total service time so it's a value in the 0 <= wrnngPrctg <= 100 range.
	 *
	 * For the rest of the parameters see TmLtchMPBttn(const uint8_t, const unsigned long int, const bool, const bool, const unsigned long int, const unsigned long int)
	 */
    HntdTmLtchMPBttn(const uint8_t &mpbttnPin, const unsigned long int &svcTime, const unsigned int &wrnngPrctg = 0, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0, const unsigned long int &strtDelay = 0);
	/**
	 * @brief See DbncdMPBttn::begin(const unsigned long int)
	 */
    virtual bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
   /**
    * @brief see DbncdMPBttn::clrStatus(bool)
    */
	void clrStatus(bool clrIsOn = true);
	/**
	 * @brief Returns the function that is set to execute every time the object's **Pilot** attribute flag **enters** the **Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffPilotPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object's Pilot is set to the **Off State**.
	 * @retval nullptr if there is no function set to execute when the object's Pilot enters the **Off State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it enters the **Pilot Off State**, including the modification of affected attribute flags. Making the function code too time-demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOffPilot();
	/**
	 * @brief Returns the function that is set to execute every time the object's **Warning** attribute flag **enters** the **Off State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOffWrnngPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object's Warning is set to the **Off State**.
	 * @retval nullptr if there is no function set to execute when the object's Warning enters the **Off State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it enters the **Warning Off State**, including the modification of affected attribute flags. Making the function code too time-demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOffWrnng();
	/**
	 * @brief Returns the function that is set to execute every time the object's **Pilot** attribute flag **enters** the **On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnPilotPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object's Pilot is set to the **On State**.
	 * @retval nullptr if there is no function set to execute when the object's Pilot enters the **On State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it enters the **Pilot On State**, including the modification of affected attribute flags. Making the function code too time-demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOnPilot();
	/**
	 * @brief Returns the function that is set to execute every time the object's **Warning** attribute flag **enters** the **On State**.
	 *
	 * The function to be executed is an attribute that might be modified by the **setFnWhnTrnOnWarnngPtr()** method.
	 *
	 * @return A function pointer to the function set to execute every time the object's Warning is set to the **On State**.
	 * @retval nullptr if there is no function set to execute when the object's Warning enters the **On State**.
	 *
	 * @warning The function code execution will become part of the list of procedures the object executes when it enters the **Warning On State**, including the modification of affected attribute flags. Making the function code too time-demanding must be handled with care, using alternative execution schemes, for example the function might resume a independent task that suspends itself at the end of its code, to let a new function calling event resume it once again.
	 */
	fncPtrType getFnWhnTrnOnWrnng();
	/**
	 * @brief Returns the current value of the pilotOn attribute flag.
	 *
	 * The pilotOn flag will be set when the isOn attribute flag is reset (~isOn), while the keepPilot attribute is set. If the keepPilot attribute is false the pilotOn will keep reset independently of the isOn flag value.
	 *
	 * @return The current value of the pilotOn flag
	 * @retval true: the pilotOn flag value is true
	 * @retval false: the pilotOn flag value is false
	 */
    const bool getPilotOn() const;
	/**
	 * @brief Returns the current value of the warningOn attribute flag.
	 *
	 * The warningOn flag will be set when the configured service time (to keep the ON signal set) is close to expiration, based on a configurable percentage of the total Service time.
	 *
	 * @return The current value of the warningOn attribute flag.
	 *
	 * @note As there is no configuration setting to keep the warning flag from working, the way to force the flag to stay set or stay reset is by configuring the accepted limits:
	 * - 0: Will keep the warningOn flag always false (i.e. will turn to true 0 ms before reaching the end of Service Time).
	 * - 100: Will keep the warningOn flag always true (i.e. will turn to true for the 100% of the Service Time).
	 */
    const bool getWrnngOn() const;
	/**
	 * @brief Sets the function that will be called to execute every time the object's **Pilot** is **reset**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object's **Pilot** is **reset**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOffPilotPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object's **Warning** is **reset**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOff)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOff Function pointer to the function intended to be called when the object's **Warning** is **reset**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOffWrnngPtr(void(*newFnWhnTrnOff)());
	/**
	 * @brief Sets the function that will be called to execute every time the object's **Pilot** is **set**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOn)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object's **Pilot is set**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOnPilotPtr(void(*newFnWhnTrnOn)());
	/**
	 * @brief Sets the function that will be called to execute every time the object's **Wrnng** is **set**.
	 *
	 * The function to be executed must be of the form **void (*newFnWhnTrnOn)()**, meaning it must take no arguments and must return no value, it will be executed only once by the object (recursion must be handled with the usual precautions). When instantiated the attribute value is set to **nullptr**.
	 *
	 * @param newFnWhnTrnOn: function pointer to the function intended to be called when the object's **Wrnng** is **set**. Passing **nullptr** as parameter deactivates the function execution mechanism.
	 */
	void setFnWhnTrnOnWrnngPtr(void(*newFnWhnTrnOn)());
	/**
	 * @brief Sets the configuration of the keepPilot service attribute.
	 *
	 * @param newKeepPilot The new setting for the keepPilot service attribute.
	 */
    void setKeepPilot(const bool &newKeepPilot);
	/**
	 * @brief See TmLtchMPBttn::setSrvcTime(const unsigned long int)
	 *
	 * @note As the warningOn attribute flag behavior is based on a percentage of the service time setting, changing the value of that service time implies changing the time amount for the warning signal service, recalculating such time as the set percentage of the new service time.
	 */
    bool setSrvcTime(const unsigned long int &newSvcTime);
	/**
	 * @brief Sets the value for the percentage of service time for calculating the warningOn flag value.
	 *
	 * The amount of **time before expiration** of service time that the warning flag must be set is defined as a percentage of the total service time so it's a value in the 0 <= wrnngPrctg <= 100 range.
	 *
	 * @param newWrnngPrctg The new percentage of service time value used to calculate the time before service time expiration to set the warningOn flag.
	 *
	 * @return Success changing the percentage to a new value
	 * @retval true the value was within range, the new value is set
	 * @retval false the value was outside range, the value change was dismissed.
	 */
	bool setWrnngPrctg (const unsigned int &newWrnngPrctg);
};

//==========================================================>>

/**
 * @brief Models an External Unlatch LDD-MPB, a.k.a. Emergency Latched Switch (**XULDD-MPB**)
 *
 * The **External released toggle switch** (a.k.a. Emergency latched), keeps the On state since the moment the signal is stable (debounced & delayed), and until an external signal is received. This kind of switch is used when an "abnormal situation" demands the push of the switch On, but a higher authority is needed to reset it to Off from a different signal source. The **On State** will then not only start a response to the exception arisen, but will be kept to flag the triggering event.
 *  Smoke, flood, intrusion alarms and "last man locks" are some examples of the use of this switch. As the external release signal can be physical or logical generated it can be implemented to be received from a switch or a remote signal of any usual kind.
 *
 * class XtrnUnltchMPBttn
 */
class XtrnUnltchMPBttn: public LtchMPBttn{
protected:
    DbncdDlydMPBttn* _unLtchBttn {nullptr};
    bool _xtrnUnltchPRlsCcl {false};

 	virtual void stOffNVURP_Do();
 	virtual void updValidUnlatchStatus();
public:
 	/**
	 * @brief Class constructor
	 *
	 * This class constructor makes specific reference to a source for the unlatch signal by including a parameter referencing an object that implements the needed getIsOn() method to get the external unlatch signal.
	 *
 	 * @param unLtchBttn Pointer to a DbncdDlydMPBttn object that will provide the unlatch source signal through it's **getIsOn()** method.
 	 *
 	 * @warning Referencing a DbncdDlydMPBttn subclass object that keeps the isOn flag set for a preset time period might affect the latching/unlatching process, as this class's objects don't check for the isOn condition of the unlatching object prior to setting it's own isOn flag.
 	 *
 	 * @note For the other parameters see DbncdDlydMPBttn(const uint8_t, const bool, const bool, const unsigned long int, const unsigned long int)
 	 *
 	 * @note Other unlatch signal origins might be developed through the unlatch() method provided.
 	 */
    XtrnUnltchMPBttn(const uint8_t &mpbttnPin,  DbncdDlydMPBttn* unLtchBttn,
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay);
    /**
     * @brief Class constructor
     *
     * This class constructor instantiates an object that relies on the **unlatch()** method invocation to release the latched MPB
     *
     * @note For the parameters see DbncdDlydMPBttn(const uint8_t, const bool, const bool, const unsigned long int, const unsigned long int)
     */
    XtrnUnltchMPBttn(const uint8_t &mpbttnPin,  
        const bool &pulledUp,  const bool &typeNO,  const unsigned long int &dbncTimeOrigSett,  const unsigned long int &strtDelay);

    /**
     * @brief See DbncdMPBttn::begin(const unsigned long int)
     */
    virtual bool begin(const unsigned long int &pollDelayMs = _StdPollDelay);
    /**
     * @brief See DbncdMPBttn::clrStatus(bool)
     */
    void clrStatus(bool clrIsOn = true);
};

//==========================================================>>
#endif   /*_BUTTONTOSWITCH_AVR_H_*/
