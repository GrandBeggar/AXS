/*
 * PowerState.cpp
 *
 * Created: 9/24/2022 5:53:36 PM
 *  Author: kevin
 */ 

#include "StateManager.h"
#include "PowerState.h"
#include "AXFManager.h"
#include "DataManager.h"
#include "IOManager.h"

// Include ClearCore last - it defines a ClearCore namespace that interferes with STD declarations.
#include "ClearCore.h"

#define POWER_START_DELAY_TIMER 2000

extern StateHolder	States;
extern DataHolder	Data;
extern IOHolder		IO;

PowerState::PowerState() {};

void PowerState::Refresh() {
	
	//	Set all required reset bits before switch/case
	ms = Milliseconds();
	
	statusInt = static_cast<Status>(status);
	Data.PowerSt.Set(statusInt);
	
	//////////////////////////////////////////////////////////////////////////
	//	Status == "OFF"
	//	WITH RESET EVENTS
	//	Control Power Risen
	//	Control Power TRUE(OFF) && Control Stop FALSE(PRESSED)
	if (status == OFF) {
		
		//	Monitor Control Stop State		// Control Stop Clear when TRUE	
		if	(!IO.ControlStop.Status.Active) {
			//	Move to the next state	
			status = READY;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//	Status == "READY" ( waiting for control power )
	//	No Reset Events
	if (status == READY) {

		// Force Machine State
		States.Machine.status = MachineState::Status::POWER_READY;

		//	Monitor control power state	//	Control Power on when FALSE
		if (!IO.ControlPower.Status.Active) {
			PowerStart = ms;
			PowerStartTarget = ms + POWER_START_DELAY_TIMER;
			status = START_DELAY;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//	Status == "START_DELAY" ( waiting for control power timer to complete )
	//	No Reset Events
	if (status == START_DELAY) {

		//	Force Machine State
		States.Machine.status = MachineState::Status::POWER_START_DELAY;

		//	Monitor power on timer
		if (PowerStartTarget <= ms) {
			status = ON;
			States.Machine.status = MachineState::Status::IDLE;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//	Status == "ON"	( ready -- only loss of control power will change state )
	//	No Reset Events
	if (status == ON) {}

}

void PowerState::Reset() {
	status = OFF;
}