/*
 * registers.h
 *
 * Created: 9/21/2022 9:15:53 PM
 *  Author: kevin
 */ 


#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include <map>

#include "ClearCore.h"

class DataMap {
	friend class AXFManager;
	friend class DataHolder;

public:

	DataMap() {};

	typedef enum {
		GETTER = 0,
		SETTER = 1,
		COUNTER = 2,
	} _GetSet;


	unsigned char	LocalBit[16];
	long int	Local;
	long int	Sent;
	long int	Remote;
	_GetSet		GetSet;
	int			Address;
	bool		Synced;
	bool		Updating;
	int			DataType;
	int			Priority;
//	int			LastUpdate;
	int			NextUpdate;

	void Set(int _value);
	void Get(int _value);
	void GS(int _value);
	void Sync();
	void Sync(int _priority);
	void SendUpdate();

	protected: 

		DataMap( _GetSet __GetSet, int _Address, int _DataType);
	
};	//	_DATA CLASS

class DataHolder {
	friend class DataMap;
	
	public:
	
	DataHolder();
	std::map<const int, DataMap *> Arr;
	
	//	HMI	|	MAIN PAGE 
	DataMap SpeedTarget;		DataMap SpeedActual;		DataMap SpeedRate;
	DataMap OffsetTarget;		DataMap OffsetIFActual;		DataMap OffsetOFActual;
	DataMap RunQtyTarget;		DataMap RunQtyActual;		DataMap RunQtyState;
	DataMap InfeedTorque;		DataMap OutfeedTorque;
	DataMap IFSpeedActual;

	//	HMI	|	INFO PAGE
	DataMap SuccessTotal;		DataMap MissedTotal;
	DataMap SuccessCurrent;		DataMap MissedCurrent;
	DataMap PouchRunTime;		DataMap PouchRTAvg;	DataMap PouchRTMax;
	DataMap ScanTime;			DataMap ScanTimeAvg;	DataMap ScanTimeMax;

	//	HMI	|	MANUAL SETUP PAGE
	DataMap JogIFManualSt;	DataMap JogOFManualSt;
	DataMap JogEnable;
	DataMap PowerSt;			DataMap MachineSt;			DataMap CycleSt;
	DataMap FaultSt;
	DataMap ProductDetectSt;
	DataMap Beacon1State;		DataMap Beacon2State;
	DataMap	SensorOnFault;		DataMap SensorOffFault;
	DataMap	IFMotorSt;			DataMap OFMotorSt;
	DataMap	PrintSignalState;
	DataMap	MessageSt;

	//	HMI	|	CYCLE SETTINGS PAGE
	DataMap CyTryTar;		DataMap CyTryAct;
	DataMap CyTryDurTar;	DataMap CyTryDurAct;	DataMap CyTryDurMod;
	DataMap CyTryPauseTar;	DataMap CyTryPauseAct;
	DataMap CyStopDlyTar;	DataMap CyStopDlyAct;	DataMap CyStopDlyMod;
	DataMap CyOverlapTar;	DataMap CyOverlapAct;	DataMap CyOverlapMod;
	DataMap CyStartTar;		DataMap CyStartAct;
	DataMap CySucPauseTar;	DataMap CySucPauseAct;	DataMap CySucPauseMod;
	DataMap CyPauseTar;		DataMap CyPauseAct;		
	DataMap CyResumeTar;	DataMap CyResumeAct;
	DataMap CyPauseDlyTar;	DataMap CyPauseDlyAct;	DataMap CyPauseDlyMod;
	
	//	HMI	|	SPEED SETTINGS
	DataMap MaxRPM;
	DataMap PwrStrtDlyTar;	DataMap PwrStrtDlyAct;

	//	HMI |	EXTERNAL DEVICE SETTINGS
	DataMap PrintSignalTar;		DataMap PrintSignalAct;	DataMap PrintSignalMod;
	DataMap PrintSignalOnTar;	DataMap PrintSignalOnAct;
	DataMap Beacon1Active;		DataMap Beacon2Active;

	//	HMI		MANUAL IO
	DataMap ForceDI6;	DataMap ForceDI7;	DataMap ForceDI8;
	DataMap ForceA9;	DataMap ForceA10;	DataMap ForceA11;	DataMap ForceA12;
	
};	//	DataHolder Class
#endif /* REGISTERS_H_ */