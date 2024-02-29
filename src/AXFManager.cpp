/*
 * AXFManager.cpp
 *
 * Created: 9/23/2022 6:47:36 PM
 *  Author: kevin
 */ 

#include <map>

#include "NvmManager.h"

#include <cstring>
#include <sam.h>

namespace ClearCore {
	
	extern NvmManager &NvmMgr;
	
}

#include "AXFManager.h"
#include "DataManager.h"
#include "CommsManager.h"
#include "StateManager.h"
#include "IOManager.h"

#define NVM_LOCATION_TO_INDEX_AXF(loc_axf) ((loc_axf) + 7168)
#define NVM_STORAGE_LOCATION 0x00070000

//	AXF Objects
Comms			Comm;
DataHolder		Data;
StateHolder		States;
IOHolder		IO;

AXFManager AXF;

AXFManager::AXFManager() : AXFReady(false) {
	AXFReady = true;
}

void AXFManager::PopulateCache() {
    // BK 9/9/20: Not sure why this was done, but it may slow down performance 
    // if the cache is disabled. 
    //NVMCTRL->CTRLA.bit.CACHEDIS0 = 1;
    //NVMCTRL->CTRLA.bit.CACHEDIS1 = 1;
    // Copy the contents of memory into a buffer
    memcpy(m_nvmPageCache, reinterpret_cast<const void *>(NVM_STORAGE_LOCATION),NVMCTRL_PAGE_SIZE);
}

void AXFManager::Init() {
	
//	m_nvmPageCache32(reinterpret_cast<int32_t *>(m_nvmPageCache));
	
	PopulateCache();
	
	//reup.
	
	//	Initialize connection protocol ( Ethernet UDP )
	ConnectionReady = Comm.CreateConnection();

	//	Initialize IO modes in Clearcore
	IO.LampRed.Ref			-> Mode(Connector::OUTPUT_DIGITAL);
	IO.LampGreen.Ref		-> Mode(Connector::OUTPUT_DIGITAL);
	IO.LampBlue.Ref			-> Mode(Connector::OUTPUT_DIGITAL);
	IO.Sensor.Ref			-> Mode(Connector::INPUT_DIGITAL);
	IO.Beacon1.Ref			-> Mode(Connector::INPUT_DIGITAL);
	IO.Beacon2.Ref			-> Mode(Connector::INPUT_DIGITAL);
	IO.Downstream.Ref		-> Mode(Connector::INPUT_DIGITAL);
	
	IO.PrintSignalSensor.Ref-> Mode(Connector::INPUT_DIGITAL);
	IO.PrintSignal.Ref		-> Mode(Connector::OUTPUT_DIGITAL);
	
	//	Set Motor Mode
	MotorMgr.MotorModeSet(MotorManager::MOTOR_ALL,Connector::CPM_MODE_A_DIRECT_B_PWM);
	
	//	Set Motor HLFB ( High Level Feedback )
	IO.InfeedMotor.Ref->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
	IO.InfeedMotor.Ref->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
	IO.OutfeedMotor.Ref->HlfbMode(MotorDriver::HLFB_MODE_HAS_BIPOLAR_PWM);
	IO.OutfeedMotor.Ref->HlfbCarrier(MotorDriver::HLFB_CARRIER_482_HZ);
	
}

void AXFManager::Refresh() {
	
	curMS = Milliseconds();

	if (nvmMS <= curMS) {
		
		NvmDone = true;
		NvmMgr.BlockRead(NvmManager::NVM_LOC_USER_START, 400, ReadData);
		nvmMS = curMS + 100;
		ReadData[0] = 1;
		byteAddress = &(ReadData[1]);
		address = reinterpret_cast<int16_t *>(byteAddress);
		address[0] = 255;
		
		byteAddress = &(ReadData[3]);
		address32 = reinterpret_cast<int32_t *>(byteAddress);
		address32[0] = 1234567890;

		NvmDone = false;
		NvmWritten = true;
		NvmMgr.BlockWrite(NvmManager::NVM_LOC_USER_START, 100, &ReadData[0]);
	}

	
	Scan.Manager.Start();
	
	//	Cycle Comms
	Comm.RefreshComms();
	
	IO.RefreshIO();
	
	Algorithms();
	
	States.Refresh();
	
	//	Manual Operations
	
	//	Run Motor(s)
	ProcessMotors();
	
	Comm.SendMessages();

	Scan.Manager.Stop();	
	Data.ScanTime.Set(Scan.Manager.Length); 
	Data.ScanTimeAvg.Set(Scan.Manager.AvgAcc);
	Data.ScanTimeMax.Set(Scan.Manager.Max);
	
//	ScanTime.LocalNew = ScanTime;
}

void AXFManager::ProcessMotors() {
	
	//	Automatic Cycle

	if (States.CycleOn || States.CycleActive) {
		IO.OutfeedMotor.Status.Active = true;
	}	
	
	if (States.CycleActive) {
		if (States.CycleOn && 
			(States.Cycle.status != CycleState::CYCLE_OFF && States.Cycle.status != CycleState::CYCLE_FAILURE) &&
			(!States.Cycle.CyclePause))
			IO.InfeedMotor.Status.Active = true;
		else
			IO.InfeedMotor.Status.Active = false;
	} 
	
	//	Manual Controls
	else if (States.ManualActive) {
		if (Data.JogIFManualSt.Local == 1 || (Data.JogIFManualSt.Local == 2 && IO.StartButton.Status.Active))
			IO.InfeedMotor.Status.Active = true;
		else
			IO.InfeedMotor.Status.Active = false;

		if (Data.JogOFManualSt.Local == 1 || (Data.JogOFManualSt.Local == 2 && IO.StartButton.Status.Active))
			IO.OutfeedMotor.Status.Active = true;
		else
			IO.OutfeedMotor.Status.Active = false;
	}
	
	//	Abort motor activation if not cycling, or in manual mode
	else {
		IO.InfeedMotor.Status.Active = false;
		IO.OutfeedMotor.Status.Active = false;
	}

}

//	Check Target Speed	//	Check Max RPM	//	Check Offset
void AXFManager::Algorithms() {
	
	//	MACHINE SPEED ACTUAL |	m/min	//
	machineMaxSpeed = Data.MaxRPM.Local;			//	meters / min
	// 40 m/m
	
	machineSpeedTarget = Data.SpeedTarget.Local;	//	% of max machine speed
	
	machineSpeedActual = machineMaxSpeed * (machineSpeedTarget * .01);
	
	machineIFSpeedActual = machineSpeedActual * (1 - Data.OffsetTarget.Local * .01);
	machineDistancePerSecond = machineSpeedActual / .06;	//	mm / second
	machineIfDistancePerSecond = machineDistancePerSecond * (1 - Data.OffsetTarget.Local * .01);
	Data.SpeedActual.Set(machineSpeedActual);
	Data.IFSpeedActual.Set(machineIFSpeedActual);
	
	//	*** AXF CALCULATION FOR REFERENCE *** Roller diameter 30mm |	RPM		//	Rounded to the nearest 5
	//	machineRPM = floor((machineSpeedActual * 200) / (30 * 3.14) + .5) / .2;
	
	//	*** AXS is using 5mm pitch pulley * 20 tooth sprocket. 100 mm/revolution | 5:1 Gearbox!!
	machineRPM = floor(((machineSpeedActual * 200) / (5*20) + .5) * 5) / .2;
	
	//	Force RPM to 10, rollers will not operate at lower speed (and so we don't end up dividing by zero)
	if (machineRPM < 10) machineRPM = 10;
	Data.OffsetOFActual.Set(machineRPM);

	//	IF RPM is based on machine RPM * 1-offset	//	Rounded to the nearest 5
	machineIfRPM = floor((machineRPM * .2 * (1 - Data.OffsetTarget.Local * .01))) / .2;
	//	Force RPM to 10, rollers will not operate at lower speed (and so we don't end up dividing by zero)
	if (machineIfRPM < 10) machineIfRPM = 10;
	Data.OffsetIFActual.Set(machineIfRPM);
	
	//	Sensor Blocked -> Timer -- How long the sensor can be blocked before faulting the machine
	cycleOverlapTarget = Data.CyOverlapTar.Local;
	if (machineDistancePerSecond > 0)
		cycleOverlapDuration = (cycleOverlapTarget / machineDistancePerSecond) * 1000;	//	convert to milliseconds
	else cycleOverlapDuration = 1000;
	Data.CyOverlapMod.Set(cycleOverlapDuration);
		
	//	Cycle Try Distance -> Timer | Infeed Roller rotations
	cycleTryTarget = Data.CyTryDurTar.Local;
	if (machineIfDistancePerSecond > 0) 
		cycleTryDuration = cycleTryTarget / machineIfDistancePerSecond * 1000;
	else cycleTryDuration = 1000;
	Data.CyTryDurMod.Set(cycleTryDuration);
	
	//	Cycle Pause Delay Distance -> Timer | Outfeed Roller rotations
	cyclePauseDelayTarget = Data.CyPauseDlyTar.Local;
	if (machineIfDistancePerSecond > 0)
		cyclePauseDelayDuration = cyclePauseDelayTarget / machineIfDistancePerSecond * 1000;
	else 
		cyclePauseDelayDuration = 0;
	Data.CyPauseDlyMod.Set(cyclePauseDelayDuration);
	
	//	CYCLE SUCCESS PAUSE DURATION | Infeed Roller
	cycleSuccessPauseTarget = Data.CySucPauseTar.Local;
	cycleSuccessPauseDuration = (Data.CyOverlapMod.Local * (cycleSuccessPauseTarget * .01));
	Data.CySucPauseMod.Set(cycleSuccessPauseDuration);
	
	//	CYCLE STOP DURATION | Infeed Roller
	Data.CyStopDlyMod.Set(cycleTryDuration);

	//	PRINT SIGNAL DURATION
	//	m/m * 1000
	//	Sensor Blocked -> Timer -- How long the sensor can be blocked before faulting the machine
	printSignalTarget = Data.PrintSignalTar.Local;
	if (machineDistancePerSecond > 0)
		printSignalDuration = (printSignalTarget / machineDistancePerSecond) * 1000;	//	convert to milliseconds
	else printSignalDuration = 1000;
	Data.PrintSignalMod.Set(printSignalDuration);
	
	if (Data.RunQtyActual.Local > Data.RunQtyTarget.Local)
		Data.RunQtyActual.Set(Data.RunQtyTarget.Local);
}

void AXFManager::ScanObj::Start() {
	StartTime = Microseconds();
}

void AXFManager::ScanObj::Stop() {
	Length = Microseconds() - StartTime;
	if (Length > Max) Max = Length;
	Scans++;
	Total += Length;
	Average = Total / Scans;
	if (Scans > 99) {
		AvgAcc = (AvgAcc + Average) / 2;
		Scans = 0;
		Total = 0;
	}

}

void AXFManager::Avg::Add(int _cur) {
	Cur = _cur;
	if (Cur > Max) Max = Cur;
	Counts++;
	Total += Cur;
	Average = Total / Counts;
	if (Counts > 99) {
		if (AvgAcc > 0) {
			AvgAcc = (AvgAcc + Average) / 2;
		}
		else
			AvgAcc = Average;
		Counts = 0;
		Total = 0;
	}
}

void AXFManager::Avg::Clear() {
	Counts = 0;
	Max = 0;
	Total = 0;
	Average = 0;
	AvgAcc = 0;
}

int AXFManager::Avg::GetAvg() {
	
	if (AvgAcc > 0) return AvgAcc;
	else return Average;
	
}

void AXFManager::Timer::Start(int _length) {

	Target = AXF.curMS + _length;
	Active = true;
	
}

bool AXFManager::Timer::Cycle(int _length) {

	if (!Active) {
		
		Start(_length);
		
	} else {
		
		return Done();
		
	}

	return false;

}

bool AXFManager::Timer::Done() {

	if (Target <= AXF.curMS) {
		Active = false;
		return true;
	} 
	else return false;

}

//	EXTERNAL VARIABLES

//	DEFINE STATIC POINTS?

//	DEFINE SYSTEM OBJECTS

//	CONSTRUCTOR	|	DEFINE SYSTEM OBJECTS

//	INTIALIZE	|	SET ANY DEFAULT STATES THAT ARE REQUIRED!
//				|	INITILIAZE ANY OTHER MANAGERS

//	READY TO GO!

//	UPDATE ROUTINES BY PRIORITY  ->
//				|	By ticks/micro/milli??

//	TRACK CYCLE RATE?? ->
//				|	How much time since last cycle?
