/*
 * AXFManager.cpp
 *
 * Created: 9/23/2022 6:47:36 PM
 *  Author: kevin
 */

#include <map>
#include <cstring>
#include <sam.h>

#include "AXFManager.h"
#include "DataManager.h"
#include "CommsManager.h"
#include "StateManager.h"
#include "IOManager.h"

constexpr double MM_PER_REV        = 125.0;
constexpr double M_PER_REV         = MM_PER_REV / 1000.0;
constexpr double GEARBOX_REDUCTION = 5.0;
constexpr double MOTOR_PULLEY      = 24.0;
constexpr double DRIVE_PULLEY      = 19.0;
constexpr double GEAR_REDUCTION    = GEARBOX_REDUCTION * (DRIVE_PULLEY / MOTOR_PULLEY);
constexpr double REDUCED_M_PER_REV = M_PER_REV / GEAR_REDUCTION;
constexpr double SCALE_OFFSET      = 8.0;
constexpr double SCALE_FACTOR      = 1.0 / SCALE_OFFSET;


#define CYCLE_STOP_DISTANCE		1000



//	AXF Objects
Comms			Comm;
DataHolder		Data;
StateHolder		States;
IOHolder		IO;

AXFManager AXF;

AXFManager::AXFManager() : AXFReady(false) {
	AXFReady = true;
}

void AXFManager::Init() {
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

	//	OUTFEED MOTOR AUTOMATIC CYCLE
	if (States.CycleOn || States.CycleActive) {
		IO.OutfeedMotor.Status.Active = true;
	}

	else if (States.ManualActive) {
		if (Data.JogOFManualSt.Local == 1 || (Data.JogOFManualSt.Local == 2 && IO.StartButton.Status.Active))
			IO.OutfeedMotor.Status.Active = true;
		else
			IO.OutfeedMotor.Status.Active = false;
	}

	//	Abort motor activation if not cycling, or in manual mode
	else {
		IO.OutfeedMotor.Status.Active = false;
	}

	//	INFEED MOTOR AUTOMATIC CYCLE
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
	}

	//	Abort motor activation if not cycling, or in manual mode
	else {
		IO.InfeedMotor.Status.Active = false;
	}

}

//	Check Target Speed	//	Check Max RPM	//	Check Offset
void AXFManager::Algorithms() {

	//	MACHINE SPEED ACTUAL |	m/min	//
	machineMaxSpeed = Data.MaxRPM.Local;			// Meters per minute -- default 40
	machineSpeedTarget = Data.SpeedTarget.Local;	// As a percentage of max speed.
	machineSpeedActual = machineMaxSpeed * (machineSpeedTarget * .01);

	machineIFSpeedActual = machineSpeedActual * (1 - Data.OffsetTarget.Local * .01);
	machineDistancePerSecond = machineSpeedActual / .06;	//	mm / second
	machineIfDistancePerSecond = machineDistancePerSecond * (1 - Data.OffsetTarget.Local * .01);
	Data.SpeedActual.Set(machineSpeedActual);
	Data.IFSpeedActual.Set(machineIFSpeedActual);

	//	*** AXS is using 5mm pitch pulley * 25 tooth sprocket. 125 mm/revolution | 5:1 Gearbox & 19/19 Drive pulley
	machineRPM = floor((machineSpeedActual / REDUCED_M_PER_REV + 0.5) * SCALE_FACTOR) / SCALE_FACTOR;

	if (machineRPM < SCALE_OFFSET) machineRPM = SCALE_OFFSET;
	Data.OffsetOFActual.Set(machineRPM);

	//	IF RPM is based on machine RPM * 1-offset	//	Rounded to the nearest 5
	machineIfRPM = floor((machineRPM * (1 - Data.OffsetTarget.Local * .01)) * SCALE_FACTOR) / SCALE_FACTOR;

	if (machineIfRPM < SCALE_OFFSET) machineIfRPM = SCALE_OFFSET;
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
	cyclePauseDelayTarget = Data.FeedPauseDelayTarget.Local;
	cyclePauseDelayDuration = (Data.CyOverlapMod.Local * (cyclePauseDelayTarget * .01));
	Data.FeedPauseDelayModified.Set(cyclePauseDelayDuration);

	//	CYCLE SUCCESS PAUSE DURATION | Infeed Roller
	cycleSuccessPauseTarget = Data.FeedPauseTarget.Local;
	cycleSuccessPauseDuration = (Data.CyOverlapMod.Local * (cycleSuccessPauseTarget * .01));
	Data.FeedPauseModified.Set(cycleSuccessPauseDuration);

	//	CYCLE STOP DURATION | Infeed Roller
	cycleStopTarget = Data.CyStopDlyTar.Local;
	if (cycleStopTarget < 1) {
		cycleStopTarget = CYCLE_STOP_DISTANCE;
	}
	if (machineIfDistancePerSecond > 0) {
		cycleStopDuration = cycleStopTarget / machineIfDistancePerSecond * 1000;
	}
	else {
		cycleStopDuration = 1000;
	}
	Data.CyStopDlyMod.Set(cycleStopDuration);


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
