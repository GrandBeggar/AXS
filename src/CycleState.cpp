/*
 * CycleState.cpp
 *
 * Created: 9/24/2022 7:01:28 PM
 *  Author: kevin
 */ 

#include "CycleState.h"
#include "MachineState.h"
#include "StateManager.h"
#include "DataManager.h"
#include "IOManager.h"

#include "ClearCore.h"

extern StateHolder States;
extern DataHolder Data;
extern IOHolder IO;

CycleState::CycleState() {}

void CycleState::Refresh(void) {

	int ms = Milliseconds();
	
	int statusInt	= static_cast<Status>(status);

	Data.CycleSt.Set(statusInt);
	
	if (status == CYCLE_OFF) {

		// initiate cycle try timer
		CycleTryDurationTarget = ms + Data.CyTryDurMod.Local;
		Data.CyTryPauseAct.Local = 0;
		Data.CyTryDurAct.Local = 0;
		Data.CyOverlapAct.Local = 0;
		CycleUpdated = false;
		PouchQtyCounted = false;
		PouchLengthCounted = false;
		CyclePause = false;
		status = CYCLE_TRY;
	}
	if (status == CYCLE_TRY ) {
		// check try timer			-> failure on expire
		Data.CyTryDurAct.Local = CycleTryDurationTarget - ms;
		
		if (IO.Sensor.Status.OSF) {
			CycleDebounceTarget = ms + 50;
		}
		if (IO.Sensor.Status.Active) {
			CycleDebounceTarget = 0;
		}
		if (!IO.Sensor.Status.Active && CycleDebounceTarget <= ms) {
			CycleOverlapDurationTarget = ms + Data.CyOverlapMod.Local;
			LastPouchStart = ms;
			CycleSucPauseTarget = ms + Data.CySucPauseMod.Local + Data.CyPauseDlyMod.Local;
			status = CYCLE_FEEDING;
		} 
		
		if (CycleTryDurationTarget <= ms) {
			status = CYCLE_FAILURE;
			CycleTryPauseTarget = ms + Data.CyTryPauseTar.Local;
		}
		// watch for sensor status	-> success on detect
		
	}
	if (status == CYCLE_FEEDING) {
		LastPouchAcc = ms - LastPouchStart;
		
		Data.CySucPauseAct.Local = CycleSucPauseTarget - ms;
		if (CycleSucPauseTarget <= ms) {
			Data.CySucPauseAct.Local = 0;
			CyclePause = false;
		}
		else {
			CyclePause = true;
		}
		

		//	Watch for sensor to clear 
		if (IO.Sensor.Status.OSR) {
			CyclePause = false;
			if (!PouchLengthCounted) {
				PouchLengthCounted = true;
				Data.PouchRunTime.Local = LastPouchAcc;
				PouchTotal += Data.PouchRunTime.Local;
				PouchCount += 1;
				PouchAvg = PouchTotal / PouchCount;
				if (Data.PouchRunTime.Local > Data.PouchRTMax.Local) 
					Data.PouchRTMax.Local = Data.PouchRunTime.Local;
				if (PouchTotal > 10) {
					if (Data.PouchRTAvg.Local > 1)
						Data.PouchRTAvg.Local = (Data.PouchRTAvg.Local + PouchAvg) / 2;
					else
						Data.PouchRTAvg.Local = PouchAvg;
					PouchTotal = 0;
					PouchCount = 0;
				}
			}
			status = CYCLE_SUCCESS;
		}
		
		
		Data.CyOverlapAct.Local = CycleOverlapDurationTarget - ms;
		if (CycleOverlapDurationTarget <= ms)
		{
			Data.CyOverlapAct.Local = 0;
			States.SensorAlwaysOn.status = StateHolder::Fault::ON;
		}
	}
	
	if (status == CYCLE_SUCCESS) {
		
		if (!CycleUpdated) {
			CycleUpdated = true;
			
			CheckSpeed();

			if (Data.RunQtyTarget.Local > 0 && Data.RunQtyState.Local > 0) {
				if (!PouchQtyCounted) {
					PouchQtyCounted = true;
					Data.RunQtyActual.Local = Data.RunQtyActual.Local + 1;
				}
				if (Data.RunQtyActual.Local >= Data.RunQtyTarget.Local) {
					Data.RunQtyActual.Set(0);
					States.Machine.StopCycle();
					States.RunQuantityLimit.status = StateHolder::Fault::ON;
				}
			}
		
			Data.SuccessCurrent.Set(Data.SuccessCurrent.Local + 1);
			Data.SuccessTotal.Set(Data.SuccessTotal.Local + 1);
			
			IO.InfeedMotor.Status.CycleError = false;
			
		}

		Reset();
		
	}
	
	if (status == CYCLE_FAILURE) {
		if (!CycleUpdated) {
			CycleUpdated = true;
			RateStart = 0;
			Data.CyTryAct.Set(Data.CyTryAct.Local + 1);
			Data.CyTryPauseAct.Local = CycleTryPauseTarget - ms;
		
			if (Data.CyTryAct.Local >= Data.CyTryTar.Local) {
				if (IO.InfeedMotor.Status.Average.GetAvg() < 7)
					States.HopperEmpty.status = StateHolder::Fault::ON;
				else {
					States.SensorAlwaysOff.status = StateHolder::Fault::ON;
					Data.MissedCurrent.Set(Data.MissedCurrent.Local + 1);
					Data.MissedTotal.Set(Data.MissedTotal.Local + 1);
				}
					
			}
		}
	
		// Reset
		if (CycleTryPauseTarget <= ms) {
			States.Cycle.status = CycleState::CYCLE_OFF;
			Data.CyTryDurAct.Set(0);
			Data.CyTryPauseAct.Set(0);
			Data.CySucPauseAct.Set(0);
		}
		
	}
	
	Data.CyOverlapAct.Sync();
	Data.CyTryDurAct.Sync();
	Data.CyTryPauseAct.Sync();
	Data.RunQtyActual.Sync();
	Data.PouchRunTime.Sync();
	Data.PouchRTAvg.Sync();
	Data.PouchRTMax.Sync();
	
}

void CycleState::Reset(void) {
	
	States.Cycle.status = CycleState::CYCLE_OFF;
	Data.CycleSt.Set(0);
	Data.CyTryAct.Set(0);
	Data.CyTryDurAct.Set(0);
	Data.CyTryPauseAct.Set(0);
	Data.CySucPauseAct.Set(0);

}

void CycleState::Success(int _ms) {
	
		Reset();
		
}

void CycleState::Failure() {

		Reset();
	
}

void CycleState::CheckSpeed() {
	
	//	Compare to time since the last pouch ran
	//	Ignore if outside a set range ( > x seconds? )
	//	Divide ms by 60000 to get cycle rate per min
	//	Push into Cycle Speed history array to get normalized speed.
	
	int _ms = Milliseconds();
	
	if (!RateStart) {
		RateStart = _ms;
		return;
	}
	else {
		RateLength = _ms - RateStart;
		RateStart = _ms;
	}
	
	RateAcc = 0; RateCount = 0;
	for (int i =9; i >= 0; i-- ) {
		if (i > 0)
			RateArr[i] = RateArr[i-1];
		else		
			RateArr[0] = RateLength;
			
		if (RateArr[i] > 0) {
			RateAcc += RateArr[i];
			RateCount += 1;
		}
	}
	
	if (RateAcc > 0 && RateCount > 0)
		Rate = 60000 / (RateAcc / RateCount);
	else
		Rate = 0;

	Data.SpeedRate.Set(Rate);
	RateLength = 0;
}