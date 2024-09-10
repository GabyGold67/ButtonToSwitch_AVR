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
#ifndef _BUTTONTOSWITCH_AVR_H_
#define _BUTTONTOSWITCH_AVR_H_

#include <Arduino.h>
#include <stdint.h>

#define _HwMinDbncTime 20   //Documented minimum wait time for a MPB signal to stabilize
#define _StdPollDelay 10
#define _MinSrvcTime 100
#define _InvalidPinNum 0xFF	// Value to give as "yet to be defined", the "Valid pin number" range and characteristics are development platform and environment related

/*---------------- xTaskNotify() mechanism related constants, argument structs, information packing and unpacking BEGIN -------*/
const uint8_t IsOnBitPos {0};
const uint8_t IsEnabledBitPos{1};
const uint8_t PilotOnBitPos{2};
const uint8_t WrnngOnBitPos{3};
const uint8_t IsVoidedBitPos{4};
const uint8_t IsOnScndryBitPos{5};
const uint8_t OtptCurValBitPos{16};

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
/*---------------- xTaskNotify() mechanism related constants, argument structs, information packing and unpacking END -------*/

// Definition workaround to let a function/method return value to be a function pointer
typedef void (*fncPtrType)();
typedef  fncPtrType (*ptrToTrnFnc)();

//===========================>> BEGIN General use function prototypes
MpbOtpts_t otptsSttsUnpkg(uint32_t pkgOtpts);
//===========================>> END General use function prototypes

//===========================>> BEGIN General use Global variables
//===========================>> END General use Global variables

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
	fdaDmpbStts _mpbFdaState {stOffNotVPP};
	TimerHandle_t _mpbPollTmrHndl {NULL};   //FreeRTOS returns NULL if creation fails (not nullptr)
	String _mpbPollTmrName {""};
	volatile bool _outputsChange {false};
	bool _prssRlsCcl{false};
	unsigned long int _strtDelay {0};
	bool _sttChng {true};
	TaskHandle_t _taskToNotifyHndl {NULL};
	TaskHandle_t _taskWhileOnHndl{NULL};
	volatile bool _validDisablePend{false};
	volatile bool _validEnablePend{false};
	volatile bool _validPressPend{false};
	volatile bool _validReleasePend{false};

   void clrSttChng();
	const bool getIsPressed() const;
	static void mpbPollCallback(TimerHandle_t mpbTmrCbArg);
	virtual uint32_t _otptsSttsPkg(uint32_t prevVal = 0);
	void _setIsEnabled(const bool &newEnabledValue);
	void setSttChng();
	void _turnOff();
	void _turnOn();
	virtual void updFdaState();
	bool updIsPressed();
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
    * @brief Returns the relevant attribute flags values for the object state encoded as a 32 bits value, required to pass current state of the object to another thread/task managing the outputs
    *
    * The inter-tasks communication mechanisms implemented on the class includes a xTaskNotify() that works as a light-weigh mailbox, unblocking the receiving tasks and sending to it a 32_bit value notification. This function returns the relevant attribute flags values encoded in a 32 bit value, according the provided encoding documented.
    *
    * @return A 32-bit unsigned value representing the attribute flags current values.
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
    * @brief Returns the current value of strtDelay attribute.
    *
    * Returns the current value of time used by the object to rise the isOn flag, after the debouncing process ends, in milliseconds. If the MPB is released before completing the debounce **and** the strtDelay time, no press will be detected by the object, and the isOn flag will not be affected. The original value for the delay process used at instantiation time might be changed with the **setStrtDelay()** method, so this method is provided to get the current value in use.
    *
    * @return The current strtDelay time in milliseconds.
    *
    * @attention The strtDelay attribute is forced to a 0 ms value at instantiation of DbncdMPBttn class objects, and no setter mechanism is provided in this class. The inherited DbncdDlydMPBttn class objects (and all it's subclasses) constructor includes a parameter to initialize the strtDelay value, and a method to set that attribute to a new value. This implementation is needed to keep backwards compatibility to olde versions of the library.
    */
	unsigned long int getStrtDelay();
   /**
	 * @brief Returns the task to be notified by the object when its output flags changes.
	 *
	 * The task handle of the task to be notified by the object when its **outputsChange** attribute flag is set (see getOutputsChange()) holds a **NULL** when the object is created. A valid task's TaskHandle_t value might be set by using the setTaskToNotify() method, and even set back to **NULL** to disable the task notification mechanism.
	 *
    * @return TaskHandle_t value of the task to be notified of the outputs change.
    * @retval NULL: there is no task configured to be notified of the outputs change.
    *
    * @note The notification is done through a **direct to task notification** using the **xTaskNotify()** RTOS macro, the notification includes passing the notified task a 32-bit notification value.
    */
	const TaskHandle_t getTaskToNotify() const;
	/**
	 * @brief Returns the task to be run (resumed) while the object is in the **On state**.
	 *
	 * Returns the task handle of the task to be **resumed** every time the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * @return The TaskHandle_t value of the task to be resumed while the object is in **On state**.
    * @retval NULL if there is no task configured to be resumed while the object is in **On state**.
    *
    * @warning ESP-IDF FreeRTOS has no mechanism implemented to notify a task that it is about to be set in **paused** state, so there is no way to that task to ensure it will be set to be paused in an orderly fashion. The task to be designated to be used by this mechanism has to be task that can withstand being interrupted at any point of it's execution, and be restarted from that same point next time the **isOn** flag is set. For tasks that might need attaching resources or other issues every time it is resumed and releasing resources of any kind before being **paused**, using the function attached by using **setFnWhnTrnOnPtr()** to gain control of the resources before resuming a task, and the function attached by using **setFnWhnTrnOffPtr()** to release the resources and pause the task in an orderly fashion, or use those functions to manage a binary semaphore for managing the execution of a task.
	 */
	const TaskHandle_t getTaskWhileOn();
	/**
	 * @brief Initializes an object instantiated by the default constructor
	 *
	 * All the parameters correspond to the non-default constructor of the class, DbncdMPBttn(const uint8_t, const bool, const bool, const unsigned long int)
	 */
	bool init(const uint8_t &mpbttnPin, const bool &pulledUp = true, const bool &typeNO = true, const unsigned long int &dbncTimeOrigSett = 0);
	/**
	 * @brief Pauses the software timer updating the computation of the object's internal flags value.
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
    /**
	 * @brief Sets the handle to the task to be notified by the object when its output attribute flags changes.
	 *
	 * When the object is created, this value is set to **NULL**, and a valid TaskHandle_t value might be set by using this method. The task notifying mechanism will not be used while the task handle keeps the **NULL** value, in which case the solution implementation will have to use any of the other provided mechanisms to test the object status, and act accordingly. After the TaskHandle value is set it might be changed to point to other task. If at the point this method is invoked the attribute holding the pointer was not NULL, the method will suspend the pointed task before proceeding to change the attribute value. The method does not provide any verification mechanism to ensure the passed parameter is a valid task handle nor the state of the task the passed pointer might be.
	 *
    * @param newTaskHandle A valid task handle of an actual existent task/thread running.
    *
    * @note As simple as this mechanism is, it's an un-expensive effective solution in terms of resources involved. The ESP-IDF FreeRTOS xTaskNotify() mechanism is used and the status of the object is passed as an encoded 32-bits value -the Notification Value- to the notified task. The **MpbOtpts_t otptsSttsUnpkg(uint32_t pkgOtpts)** function is provided to decode the 32-bit value into boolean values for each of the pertaining attribute flags state of the object.
    */
	void setTaskToNotify(const TaskHandle_t &newTaskHandle);    
	/**
	 * @brief Sets the task to be run while the object is in the **On state**.
	 *
	 * Sets the task handle of the task to be **resumed** when the object enters the **On state**, and will be **paused** when the  object enters the **Off state**. This task execution mechanism dependent of the **On state** extends the concept of the **Switch object** far away of the simple turning On/Off a single hardware signal, attaching to it all the task execution capabilities of the MCU.
	 *
	 * If the existing value for the task handle was not NULL before the invocation, the method will verify the Task Handle was pointing to a deleted or suspended task, in which case will proceed to **suspend** that task before changing the Task Handle to the new provided value.
	 *
	 * Setting the value to NULL will disable the task execution mechanism.
    *
    *@note The method does not implement any task handle validation for the new task handle, a valid handle to a valid task is assumed as parameter.
    *
    * @note Consider the implications of the task that's going to get suspended every time the MPB goes to the **Off state**, so that the the task to be run might be interrupted at **any** point of its execution. This implies that the task must be designed with that consideration in mind to avoid dangerous situations generated by a task not completely done when suspended.
    *
    * @warning Take special consideration about the implications of the execution **priority** of the task to be executed while the MPB is in **On state** and its relation to the priority of the calling task, as it might affect the normal execution of the application.
	 */    
	virtual void setTaskWhileOn(const TaskHandle_t &newTaskHandle);
};

//==========================================================>>

#endif   /*_BUTTONTOSWITCH_AVR_H_*/
