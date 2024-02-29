/*
 * MachineState.cpp
 *
 * Created: 9/24/2022 6:18:09 PM
 *  Author: kevin
 */ 

#include "DataManager.h"
#include "MachineState.h"
#include "StateManager.h"
#include "IOManager.h"

#include "ClearCore.h"

extern DataHolder Data;
extern StateHolder States;
extern IOHolder IO;

MachineState::MachineState()
{
	Init();
}

void MachineState::Refresh() {
	
	int ms = Milliseconds();
	statusInt = static_cast<Status>(status);

	Data.MachineSt.Set(statusInt);
	RefreshLamp(Lamps[statusInt]);
	
	//////////////////////////////////////////////////////////////////////////
	//	Status "POWER_OFF"
	//	RESET / CONTROL EVENTS
	//		First 3 states are forced by Power State
	if (status == POWER_OFF) {}
	if (status == POWER_READY) {}
	if (status == POWER_START_DELAY) {
		if (States.PowerOn)
			status = IDLE;
	}
	//////////////////////////////////////////////////////////////////////////
	//	Status "IDLE"
	//	FALLBACK EVENTS
	//		Cycle Stopped
	if (status == IDLE) {
		CycleStartActual = 0;
		CycleStopActual = 0;
		
		// Watch for Control Start Button OSR
		if (IO.StartButton.Status.OSR) {
			CycleStartTarget = ms + Data.CyStartTar.Local;
			status = CYCLE_START_DELAY;
		}
	}
	//////////////////////////////////////////////////////////////////////////
	//	Status "CYCLE START DELAY"
	//	FALLBACK EVENTS
	//		START BUTTON OSR WILL CANCEL START
	if (status == CYCLE_START_DELAY) {
		//	Fall back to idle if start button is pressed again
		if (IO.StartButton.Status.OSR && Data.MachineSt.Local == 4)
			status = IDLE;
		CycleStartActual = CycleStartTarget - ms;
		if (CycleStartTarget <= ms) {
			CycleStartActual = 0;
			status = CYCLE_ACTIVE;
		}
	}
	
	//////////////////////////////////////////////////////////////////////////
	//	Status "CYCLE ACTIVE"
	//	INTERRUPT CYCLE EVENTS
	//		DOWNSTREAM SENSOR -> CYCLE STOP -> CYCLE PAUSE ( CYCLE STOP MONITORS DOWNSTREAM )
	//		TRAY QUANTITY -> CYCLE STOP
	//	REFRESH EVENTS
	//		CYCLE STATE
	if (status == CYCLE_ACTIVE) {
		if (IO.StartButton.Status.OSR) {
			status = CYCLE_CLEARING;
		}
	}
	
	if (status == CYCLE_CLEARING) {
		if (IO.Sensor.Ref->State()) {
			CycleStopTarget = ms + Data.CyStopDlyMod.Local;
			status = CYCLE_STOPPING;
		}
	}
	
	//	Waiting for pouches to clear before stopping.. 
	if (status == CYCLE_STOPPING) {
		CycleStopActual = CycleStopTarget - ms;
		if (CycleStopTarget <= ms) {
			CycleStopActual = 0;
			status = IDLE;
		}
	}
	
	//	Only a downstream delay can get me here.. 
	if (status == CYCLE_PAUSE) {
		
	}
	
	//	Force into state when ManualActive is true
	if (status == MANUAL_OPERATION || States.ManualActive) status = MANUAL_OPERATION;
	//	

	if (status == MACHINE_MESSAGE)		status = MACHINE_MESSAGE;

	if (status == MACHINE_FAULTED)		status = MACHINE_FAULTED;

	Data.CyStartAct.Set(CycleStartActual);
	Data.CyStopDlyAct.Set(CycleStopActual);
	
}

void MachineState::Reset() {
	status = POWER_OFF;
}

void MachineState::StopCycle() {
	status = CYCLE_STOPPING;
}

void MachineState::Idle() {
	status = IDLE;
}

void MachineState::RefreshLamp(Lamp _lamp) {
	
	int ms = Milliseconds();
	
	// Check if Blink is required
	if (_lamp.BlinkEnable) {
		 //	Check if blink timer is active
		if (BlinkOn <= ms && BlinkOff <= ms) {
			BlinkActive = true;
			BlinkOn = Milliseconds() + 1000;
			BlinkOff = BlinkOn + 1000;
		}
		if (BlinkActive && BlinkOn <= ms)
			BlinkActive = false;
	} else BlinkActive = false;
	if (BlinkActive) {
		IO.LampRed.Ref	->State(!_lamp.RedBlink);
		IO.LampGreen.Ref->State(!_lamp.GreenBlink);
		IO.LampBlue.Ref	->State(!_lamp.BlueBlink);
	} else {
		IO.LampRed.Ref	->State(!_lamp.RedSolid);
		IO.LampGreen.Ref->State(!_lamp.GreenSolid);
		IO.LampBlue.Ref	->State(!_lamp.BlueSolid);
	}	
}

void MachineState::Init() {
	// POWER_OFF	[0]
	Lamps[0].RedSolid = true;
	
	//	POWER_READY	[1]
	Lamps[1].RedSolid = true;
	Lamps[1].GreenSolid = true;
	
	//	POWER_START_DELAY	[2]
	Lamps[2].RedSolid = true;
	Lamps[2].GreenSolid = true;
	Lamps[2].GreenBlink = true;
	Lamps[2].BlinkEnable = true;
	
	//	IDLE
	Lamps[3].GreenSolid = true;
	
	//	CYCLE_START
	Lamps[4].GreenSolid = true;
	Lamps[4].BlueBlink = true;
	Lamps[4].BlinkEnable = true;
	
	//	CYCLE_RUN
	Lamps[5].BlueSolid = true;
	
	//	CYCLE CLEARING
	Lamps[6].GreenSolid = true;
	Lamps[6].BlueBlink = true;
	Lamps[6].BlinkEnable = true;

	//	CYCLE STOPPING
	Lamps[7].GreenSolid = true;
	Lamps[7].BlueBlink = true;
	Lamps[7].BlinkEnable = true;
	
	//	CYCLE PAUSE
	Lamps[8].BlueSolid = true;
	Lamps[8].BlinkEnable = true;
	
	//	MANUAL
	Lamps[10].RedSolid = true;
	Lamps[10].GreenSolid = true;
	Lamps[10].BlueSolid = true;
	
	//	FAULT
	Lamps[15].RedSolid = true;
	Lamps[15].BlinkEnable = true;	
}