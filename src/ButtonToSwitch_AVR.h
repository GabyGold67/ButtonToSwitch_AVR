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
int findMCD(int a, int b);

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
	//! BEGIN Test related variables 
	static DbncdMPBttn* test_DmpbPtr;
	//! END Test related variables 

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
	unsigned long int _lstPollTime{0};	//! Timer1 use related attribute
	fdaDmpbStts _mpbFdaState {stOffNotVPP};
	DbncdMPBttn* _mpbInstnc{nullptr};	//! Timer1 use related attribute
	volatile bool _outputsChange {false};
	uint32_t _outputsChangeCnt{0};
	unsigned long int _pollPeriodMs{0};	//! Timer1 use related attribute
	bool _prssRlsCcl{false};
	unsigned long int _strtDelay {0};
	bool _sttChng {true};
	bool _updTmrAttchd{false};//! Timer1 use related attribute
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
	void _setLstPollTime(const unsigned long int &newLstPollTIme);	//! Implemented only to avoid direct manipulation of the _lstPollTime attribute
	void setSttChng();
	void _turnOff();
	void _turnOn();
	virtual void updFdaState();
	bool updIsPressed();
	unsigned long int _updTmrsMCDCalc(DbncdMPBttn** mpbsLstPtr);
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
	 * @param pollDelayMs (Optional) unsigned long integer (ulong), the time between polls in milliseconds.
	 *
	 * @return Boolean indicating if the object could be attached to a timer.
	 * @retval true: the object could be attached to a timer -or it was already attached to a timer when the method was invoked-.
	 * @retval false: the object could not create the needed timer, or the object could not be attached to it.
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
	 * @brief Detaches the object from the timer that monitors the input pins, compute and updates the object's status. The timer daemon entry is deleted for the object.
	 *
	 * The immediate detachment of the object from the timer that keeps it's state updated implies that the object's state will be kept, whatever that state is it. If a certain status is preferred some of the provided methods should be used for that including clrStatus(), resetFda(), disable(), setIsOnDisabled(), etc. Also consider that if a task is set to be executed while the object is in **On state**, the **end()** invocation wouldn't affect that task execution state.
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

#endif   /*_BUTTONTOSWITCH_AVR_H_*/
