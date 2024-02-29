/*
 *	State.cpp
 *
 *	Created: 9/20/2022 1:03:06 PM
 *  Author: Kevin Jinkerson
 *
 *	Definition:
 *	
 */ 

#include "AXFManager.h"
#include "StateManager.h"
#include "DataManager.h"
#include "IOManager.h"

#include "ClearCore.h"

extern DataHolder Data;
extern IOHolder IO;
extern StateHolder States;

StateHolder::StateHolder() {};

void StateHolder::Refresh() {

	//	Faults
	CheckFaults();
	
	ProcessSensor();
	
	ProcessPrintSignal();
	
	Power.Refresh();
	Machine.Refresh();

	if (CycleActive)
		Cycle.Refresh();
	else
		Cycle.Reset();

	//	Override States
	ProcessOverrides();


		
}

void StateHolder::ProcessOverrides() {
	
	//	Power State
	if (IO.ControlPower.Status.OSF || 
		(IO.ControlPower.Status.Active && IO.ControlStop.Status.Active))
		States.Power.Reset();
	
	//	Power State -> Machine State
	if (Power.status == PowerState::OFF)
		States.Machine.Reset();
	if (Power.status == PowerState::READY)
		States.Machine.status == MachineState::POWER_READY;
	if (Power.status == PowerState::START_DELAY)
		States.Machine.status == MachineState::POWER_START_DELAY;
	if (Power.status == PowerState::ON)
		PowerOn = true;
	else
		PowerOn = false;

	//	Machine Clear ( OK to move )
	if (PowerOn && !MachineFaulted)
		MachineClear = true;
	else
		MachineClear = false;
	
	//	Jog is enabled - manual controls active - don't monitor for most faults ( only motor status )
	if (Data.JogEnable.Local > 0 && MachineClear)
		ManualActive = true;
	else
		ManualActive = false;
	if (ManualActive)
		States.Machine.status == MachineState::MANUAL_OPERATION;
	else if (!ManualActive && Machine.status == MachineState::MANUAL_OPERATION)
		States.Machine.Idle();

	//	Faults -> Machine State
	if (PowerOn && MachineFaulted)
		States.Machine.status == MachineState::MACHINE_FAULTED;
	else if (PowerOn && MachineMessage)
		States.Machine.status == MachineState::MACHINE_MESSAGE;
	else if ((!MachineFaulted && Machine.status == MachineState::MACHINE_FAULTED) ||
			 (!MachineMessage && Machine.status == MachineState::MACHINE_MESSAGE))
		States.Machine.Idle();

	//	Run outfeed motor + Conveyor
	if (!MachineFaulted && !MachineMessage && (
		Machine.status == MachineState::CYCLE_START_DELAY ||
		Machine.status == MachineState::CYCLE_ACTIVE ||
		Machine.status == MachineState::CYCLE_CLEARING ||
		Machine.status == MachineState::CYCLE_STOPPING))
		CycleOn = true;
	else {
		CycleOn = false;
		Cycle.RateStart = 0;
	}
		
	if (!MachineFaulted && !MachineMessage && (
		Machine.status == MachineState::CYCLE_ACTIVE ||
		Machine.status == MachineState::CYCLE_CLEARING))	
		States.CycleActive = true;
	else
		States.CycleActive = false;
		
	if (!States.CycleActive) Data.PrintSignalState.Set(0);
		
	if (Data.IFMotorSt.Local == 2) MotorInfeed.status = StateHolder::Fault::ON;
	if (Data.OFMotorSt.Local == 2) MotorOutfeed.status = StateHolder::Fault::ON;
	
	
}

void StateHolder::ProcessPrintSignal() {
	
	int ms = Milliseconds();
	
	if (Data.PrintSignalState.Local == 0) {
		PrintSignalTimerTarget = 0;
		if (States.CycleActive) 
			Data.PrintSignalState.Set(1);
	}

	if (Data.PrintSignalState.Local == 1) {
		if (!IO.PrintSignalSensor.Ref->State()) {
			Data.PrintSignalState.Set(2);
			PrintSignalTimerTarget = ms + Data.PrintSignalMod.Local;
		}
	}
	
	if (Data.PrintSignalState.Local == 2) {
		Data.PrintSignalAct.Set(PrintSignalTimerTarget - ms);
		if (PrintSignalTimerTarget <= ms) {
			Data.PrintSignalState.Set(3);
			Data.PrintSignalAct.Set(0);
			PrintSignalOnTimerTarget = ms + Data.PrintSignalOnTar.Local;
		}
	}
	if (Data.PrintSignalState.Local == 3) {
		Data.PrintSignalOnAct.Set(PrintSignalOnTimerTarget - ms);
		if (PrintSignalOnTimerTarget <= ms) {
			Data.PrintSignalState.Set(4);
			Data.PrintSignalOnAct.Set(0);
		}
	}
	
	if (Data.PrintSignalState.Local == 4) {
		if (IO.PrintSignalSensor.Ref->State())
			Data.PrintSignalState.Set(0);
	}

	
	if (Data.PrintSignalState.Local == 3) 
		IO.PrintSignal.Ref	->State(true);
	else 
		IO.PrintSignal.Ref	->State(false);
	

}

void StateHolder::ProcessSensor() {
	
	if (States.CycleActive && (Data.FaultSt.Local == 1 || Data.FaultSt.Local == 2)) {
		if (Data.FaultSt.Local == 1) {
			Data.ProductDetectSt.Set(3);
		}
		if (Data.FaultSt.Local == 2) {
			Data.ProductDetectSt.Set(4);
		}
	}
	else if (!IO.Sensor.Status.Active)
		Data.ProductDetectSt.Set(2);
	else if (CycleOn && IO.Sensor.Status.Active)
		Data.ProductDetectSt.Set(1);
	else
		Data.ProductDetectSt.Set(0);
}

void StateHolder::CheckFaults() {
	if (SensorAlwaysOff.Check(2)	||
		SensorAlwaysOn.Check(1)		||
		MotorInfeed.Check(5)		||
		MotorOutfeed.Check(6)		||
		Beacon1.Check(3)			||
		Beacon2.Check(4)
		) 
	{

		Machine.status = MachineState::MACHINE_FAULTED;
		MachineFaulted = true;
	} else {
		MachineFaulted = false;
	}
	
	if (HopperEmpty.Check(7)	||
		RunQuantityLimit.Check(8)
		)
	{
		Machine.status = MachineState::MACHINE_MESSAGE;
		MachineMessage = true;
	} else {
		MachineMessage = false;
	}
}

bool StateHolder::Fault::Check(int FaultId) {
	
	if (status == ON) {
		if (IO.StartButton.Status.Active && (States.MachineFaulted or States.MachineMessage)) {
			Data.FaultSt.Set(0);
			status = OFF;
		} 
		else {
			Data.FaultSt.Set(FaultId);
			return true;
		}
	}
	return false;
	
}


void StateHolder::Fault::Faulted() {
	
	status = ON;
	
}


