/*
 * IOManager.cpp
 *
 * Created: 10/5/2022 3:48:55 PM
 *  Author: kevin
 */ 

#include "DataManager.h"
#include "IOManager.h"
#include "StateManager.h"

extern DataHolder Data;
extern StateHolder States;

IOHolder::IOHolder() {
	ControlPower.Ref	= &ConnectorDI6;	ControlPower.Force	= &Data.ForceDI6;
	StartButton.Ref		= &ConnectorDI7;	StartButton.Force	= &Data.ForceDI7;
	ControlStop.Ref		= &ConnectorDI8;	ControlStop.Force	= &Data.ForceDI8;
	Sensor.Ref			= &ConnectorA9;		Sensor.Force		= &Data.ForceA9;
	Beacon1.Ref			= &ConnectorA12;	Beacon1.Force		= &Data.ForceA12;
	Beacon2.Ref			= &ConnectorA11;	Beacon2.Force		= &Data.ForceA11;
	Downstream.Ref		= &ConnectorIO1;
	
	PrintSignalSensor.Ref	= &ConnectorA10;
	PrintSignal.Ref			= &ConnectorIO2;
	
	LampRed.Ref			= &ConnectorIO3;
	LampGreen.Ref		= &ConnectorIO4;
	LampBlue.Ref		= &ConnectorIO5;
	
	InfeedMotor.Ref			= &ConnectorM0;
	InfeedMotor.Related		= &Data.IFMotorSt;
	InfeedMotor.Force		= &Data.InfeedTorque;
	
	OutfeedMotor.Ref		= &ConnectorM1;
	OutfeedMotor.Related	= &Data.OFMotorSt;
	OutfeedMotor.Force		= &Data.OutfeedTorque;

}

void IOHolder::RefreshIO() {

	// Consider adding each IO point to an array, and cycle through the array of each object type..
	
	RefreshInput(&ControlPower);
	RefreshInput(&ControlStop);
	RefreshInput(&StartButton);
	RefreshInput(&Sensor);
	RefreshInput(&PrintSignalSensor);

	if (Data.OffsetIFActual.Local < 1)
		InfeedMotor.Status.SetPoint	= 50;
	else
		InfeedMotor.Status.SetPoint	= Data.OffsetIFActual.Local;

	if (Data.OffsetOFActual.Local < 1)
		OutfeedMotor.Status.SetPoint = 50;
	else
		OutfeedMotor.Status.SetPoint = Data.OffsetOFActual.Local;

	RefreshMotor(&InfeedMotor);
	//	Gate torque averaging with x-ms Timer
	
	RefreshMotor(&OutfeedMotor);
	
}
template <class I>
void IOHolder::RefreshInput(IoObject<I> * _input) {
	//	PrevState == Active
	_input->Status.PrevState = _input->Status.Active;

	//	Check ActiveState
	if (_input->Force->Local == 0)
		_input->Status.Active = _input->Ref->State();
	
	// Check Force Status
	if (_input->Force->Local == 1) _input->Status.Active = 1;
	if (_input->Force->Local == 2) _input->Status.Active = 0;
	
	//	If New State->Rising/Falling
	if (_input->Status.Active != _input->Status.PrevState) {
		if (_input->Status.Active)
			_input->Status.OSR = true;
		else 
			_input->Status.OSF = true;
	} else {
		_input->Status.OSR = false;
		_input->Status.OSF = false;
	}
}

template <class M>
void IOHolder::RefreshMotor(IoObject<M> * _motor) {

	if (_motor->Status.Active && States.PowerOn) {
		_motor->Ref->MotorInAState(false);
 		_motor->Ref->MotorInBDuty(_motor->Status.SetPoint / 5);

		_motor->Ref->EnableRequest(true);
		
		_motor->Related->Set(1);
		
		if (_motor->Ref->MOTOR_MOVING)
			_motor->Status.Enabled = true;
	
		if (_motor->Status.Enabled && States.PowerOn) {

			_motor->Status.Feedback = abs(_motor->Ref->HlfbPercent());
			
			if (_motor->Status.TorqueAverageDelay.Cycle(1)) {
		
				if (_motor->Ref->MOTOR_MOVING)
					_motor->Status.Average.Add(_motor->Status.Feedback);
				else if (_motor->Ref->MOTOR_ENABLING)
					_motor->Status.Average.Clear();
				if (_motor->Status.Average.GetAvg() > 0)
					_motor->Force->Set(_motor->Status.Average.GetAvg());
			}			
			
			if (_motor->Ref->StatusReg().bit.AlertsPresent) {
				//_motor->Related->Set(2);
			}
			if (_motor->Ref->StatusReg().bit.MotorInFault) {
				//	Fault
				//_motor->Related->Set(2);
			}

			
			if (_motor->Status.Average.GetAvg() > 30) {
				if (_motor->Status.TimeoutTarget == 0)
					_motor->Status.TimeoutTarget = AXF.curMS + 250;
				else if (_motor->Status.TimeoutTarget <= AXF.curMS) {
					//_motor->Related->Set(2);
					_motor->Status.Average.Clear();
				}
			} else 
				_motor->Status.TimeoutTarget = 0;
		}
	} else {
		_motor->Ref->EnableRequest(false);
		_motor->Ref->MotorInAState(false);
		_motor->Ref->MotorInBState(false);
		_motor->Ref->MotorInBDuty(0);
		_motor->Status.Actual = 0;
		_motor->Status.SetPoint = 0;
		_motor->Status.Enabled = false;
		_motor->Related->Set(0);
		_motor->Status.TimeoutTarget = 0;
		_motor->Status.Average.Clear();
	}
}
