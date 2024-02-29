/**
	FILE	|	State.h
	CREATED |	Created: 9/20/2022 1:02:38 PM
	AUTHOR	|	Kevin Jinkerson
	BRIEF	|	State Machine Class
	DESC	|	The state machine class tracks a set state sequence
				ie: PowerState, MainState, Cycle State
				
				These states can then be used to perform, or block actions throughout the program.
				
				Having a master class will keep all state classes within a similar scope.
				
				State ENUM may be best to override define within the derived state machine.
				
				Follow clearpath examples for including timer/refresh functions within the state machine
				class.
				
				( Connector.h/.cpp->DigitalIn are good examples of a base/derived implementation )
 **/

#ifndef STATE_H_
#define STATE_H_

#include "MachineState.h"
#include "PowerState.h"
#include "CycleState.h"
#include "DataManager.h"

class StateHolder {
public:
	friend class AXFManager;
	friend class MachineState;
	friend class PowerState;
	friend class CycleState;

	StateHolder();
	PowerState Power;
	MachineState Machine;
	CycleState Cycle;

	bool MachineFaulted;
	bool MachineMessage;
	bool MachineClear;	//	Power On && Not Faulted	= Allow Cycle/Allow Manual
	bool CycleOn;
	bool CycleActive;
	bool PrintSignalActive;
	bool PowerOn;
	bool ManualActive;
	bool DownstreamPause;
	
	int PrintSignalState;
	
	void Refresh();
	
	class Fault {
		public:
		
		typedef enum {
			OFF = 0,
			ON = 1
		} Status;
		
		Status status;
		int statusInt;
		
		bool Check(int FaultId);	// Data Ref
		
		void Faulted();
	};
	
	Fault SensorAlwaysOn;
	Fault SensorAlwaysOff;
	Fault MotorInfeed;
	Fault MotorOutfeed;
	Fault Beacon1;
	Fault Beacon2;
	Fault HopperEmpty;
	Fault RunQuantityLimit;

private:

	void CheckFaults();
	void ProcessOverrides();
	void ProcessSensor();
	void ProcessPrintSignal();
	
	int PrintSignalTimerTarget;
	int PrintSignalOnTimerTarget;
	
	
};	//	StateHolder Class

#endif /* STATE_H_ */