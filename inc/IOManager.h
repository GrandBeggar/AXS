/*
 * IOManager.h
 *
 * Created: 10/5/2022 3:48:43 PM
 *  Author: kevin
 */ 

#ifndef IOMANAGER_H_
#define IOMANAGER_H_

#include "ClearCore.h"
#include "AXFManager.h"

extern AXFManager AXF;

template <class T>
class IoObject {
	public: 
	
	struct IOStatus {
		bool Active;
		bool PrevState;
		bool OSR;
		bool OSF;
		bool Enabled;
		bool CycleError;
		int SetPoint;
		int Actual;
		int Feedback;
		int TimeoutTarget;
		AXFManager::Avg Average;
		AXFManager::Timer TorqueAverageDelay;
	};
	
	T * Ref;
	
	DataMap * Related;
	DataMap * Force;
	
	IOStatus Status;
	
	
};

class IOHolder {
	public:
	
	IOHolder();
	
	template <class I>
	void RefreshInput(IoObject<I> * _input);
	
	void RefreshIO();
	
	void RefreshMotors();
	
	template <class M>
	void RefreshMotor(IoObject<M> * _motor);
	
	IoObject<DigitalIn>			ControlPower;
	IoObject<DigitalIn>			ControlStop;
	IoObject<DigitalIn>			StartButton;
	IoObject<DigitalInAnalogIn>	Sensor;
	IoObject<DigitalInAnalogIn>	Beacon1;
	IoObject<DigitalInAnalogIn>	Beacon2;
	IoObject<DigitalInOut>	Downstream;

	IoObject<DigitalInAnalogIn>		PrintSignalSensor;
	IoObject<DigitalInOut>			PrintSignal;
	
	
	IoObject<DigitalInOut>	LampRed;
	IoObject<DigitalInOut>	LampGreen;
	IoObject<DigitalInOut>	LampBlue;
	
	IoObject<MotorDriver>	InfeedMotor;
	IoObject<MotorDriver>	OutfeedMotor;
};

#endif /* IOMANAGER_H_ */