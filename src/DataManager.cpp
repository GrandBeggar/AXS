/*
 * registers.cpp
 *
 * Created: 9/22/2022 2:55:41 PM
 *  Author: kevin
 */

#include "DataManager.h"
#include "CommsManager.h"

#define SYNC_MESSAGE_DELAY 1000
#define DESYNC_MESSAGE_DELAY 50

extern Comms Comm;

//	DataMap Constructor -- Create instance with parameters
DataMap::DataMap( _GetSet __GetSet, int _Address, int _DataType)
:	GetSet(__GetSet),
	Address(_Address),
	Synced(false),
	DataType(_DataType)
	{};

//	Set value from local program
void DataMap::Set(int _value) {
	Local = _value;
	Sync();
}

//	Set Remote Value
void DataMap::Get(int _value) {
	Remote = _value;
	Sync();
}

void DataMap::GS(int _value) {
	Local = _value;
	Remote = _value;
	Sync(2);
}

void DataMap::Sync() {
	Sync(0);
}

//	Update Status and send to HMI ( triggered from both Set and Get )
void DataMap::Sync(int _priority) {

	if (Priority)	
		_priority = Priority;

	//	If not in sync, check if it is already updating
	if (Local != Remote) {
		Synced = false;
		if (!Updating || Local != Sent) {
			Updating = true;
			if (_priority != 1)
				_priority = 2;
		}
	} 
	//	If in sync, send handshake on low priority
	else {
		Synced = true;
		Updating = false;
		Sent = Local;
	}
	Comm.AddMessage(this,_priority);
}

DataHolder::DataHolder() {
	//	GetSet	Address	DataType[1-3]	Frequency	
	//	HMI	|	MAIN PAGE	//////////////////////////////////////////////////////////////////////////
	SpeedTarget		= DataMap(DataMap::GETTER,	20,	1);	Arr[20] = &SpeedTarget;
	SpeedActual		= DataMap(DataMap::SETTER,	21,	2);	Arr[21] = &SpeedActual;	
	SpeedRate		= DataMap(DataMap::SETTER,	22,	2);	Arr[22] = &SpeedRate;
	OffsetTarget	= DataMap(DataMap::GETTER,	24,	1);	Arr[24] = &OffsetTarget;
	OffsetIFActual	= DataMap(DataMap::SETTER,	25,	2);	Arr[25] = &OffsetIFActual;
	OffsetOFActual	= DataMap(DataMap::SETTER,	26,	2);	Arr[26] = &OffsetOFActual;
	RunQtyTarget	= DataMap(DataMap::GETTER,	27,	2);	Arr[27] = &RunQtyTarget;
	RunQtyActual	= DataMap(DataMap::COUNTER,	28,	2);	Arr[28] = &RunQtyActual;
	RunQtyState		= DataMap(DataMap::GETTER,	29,	1);	Arr[29] = &RunQtyState;
	InfeedTorque	= DataMap(DataMap::SETTER,	30,	1);	Arr[30] = &InfeedTorque;
	OutfeedTorque	= DataMap(DataMap::SETTER,	31,	1);	Arr[31] = &OutfeedTorque;
	IFSpeedActual   = DataMap(DataMap::SETTER,	32,	1);	Arr[32] = &IFSpeedActual;

	//	HMI	|	INFO PAGE	//////////////////////////////////////////////////////////////////////////
	SuccessTotal	= DataMap(DataMap::COUNTER,	50,	3);	Arr[50] = &SuccessTotal;	
	MissedTotal		= DataMap(DataMap::COUNTER,	52,	3);	Arr[52] = &MissedTotal;	
	SuccessCurrent	= DataMap(DataMap::COUNTER,	54,	3);	Arr[54] = &SuccessCurrent;	
	MissedCurrent	= DataMap(DataMap::COUNTER,	56,	3);	Arr[56] = &MissedCurrent;
	PouchRunTime	= DataMap(DataMap::COUNTER,	60,	2);	Arr[60] = &PouchRunTime;
	PouchRTAvg		= DataMap(DataMap::COUNTER,	61,	2);	Arr[61] = &PouchRTAvg;
	PouchRTMax		= DataMap(DataMap::COUNTER,	62,	2);	Arr[62] = &PouchRTMax;
	ScanTime		= DataMap(DataMap::COUNTER,	63,	2);	Arr[63] = &ScanTime;
	ScanTimeAvg		= DataMap(DataMap::COUNTER,	64,	2);	Arr[64] = &ScanTimeAvg;
	ScanTimeMax		= DataMap(DataMap::COUNTER,	65,	2);	Arr[65] = &ScanTimeMax;
	ScanTimeMax.Priority = 1;	ScanTimeAvg.Priority = 1;	ScanTime.Priority = 1;
	PouchRunTime.Priority = 1;	PouchRTAvg.Priority = 1;	PouchRTMax.Priority = 1;

	//	HMI	|	Machine States	//////////////////////////////////////////////////////////////////////////
	JogIFManualSt	= DataMap(DataMap::GETTER,	80,	1);	Arr[80] = &JogIFManualSt;
	JogOFManualSt	= DataMap(DataMap::GETTER,	81,	1);	Arr[81] = &JogOFManualSt;
	JogEnable		= DataMap(DataMap::GETTER,	83,	1);	Arr[83] = &JogEnable;
	PowerSt			= DataMap(DataMap::SETTER,	84,	1);	Arr[84] = &PowerSt;
	MachineSt		= DataMap(DataMap::SETTER,	85,	1);	Arr[85] = &MachineSt;
	CycleSt			= DataMap(DataMap::SETTER,	86,	1);	Arr[86] = &CycleSt;
	FaultSt			= DataMap(DataMap::SETTER,	87,	1);	Arr[87] = &FaultSt;
	ProductDetectSt	= DataMap(DataMap::SETTER,	88,	1);	Arr[88] = &ProductDetectSt;
	Beacon1State	= DataMap(DataMap::SETTER,	89,	1);	Arr[89] = &Beacon1State;
	Beacon2State	= DataMap(DataMap::SETTER,	90,	1);	Arr[90] = &Beacon2State;
	IFMotorSt		= DataMap(DataMap::SETTER,	91,	1);	Arr[91] = &IFMotorSt;
	OFMotorSt		= DataMap(DataMap::SETTER,	92,	1);	Arr[92] = &OFMotorSt;
	PrintSignalState= DataMap(DataMap::SETTER,	93,	1);	Arr[93] = &PrintSignalState;

	MessageSt		= DataMap(DataMap::SETTER,	94,	1);	Arr[94] = &MessageSt;

	//	CYCLE SETTINGS			//////////////////////////////////////////////////////////////////////////
	CyTryTar		= DataMap(DataMap::GETTER,	110,2);	Arr[110] = &CyTryTar;
	CyTryAct		= DataMap(DataMap::SETTER,	111,2);	Arr[111] = &CyTryAct;
	CyTryDurTar		= DataMap(DataMap::GETTER,	112,2);	Arr[112] = &CyTryDurTar;
	CyTryDurAct		= DataMap(DataMap::SETTER,	113,2);	Arr[113] = &CyTryDurAct;
	CyTryDurMod		= DataMap(DataMap::SETTER,	114,2);	Arr[114] = &CyTryDurMod;
	CyTryPauseTar	= DataMap(DataMap::GETTER,	115,2);	Arr[115] = &CyTryPauseTar;
	CyTryPauseAct	= DataMap(DataMap::SETTER,	116,2);	Arr[116] = &CyTryPauseAct;
	CyStopDlyTar	= DataMap(DataMap::GETTER,	117,2);	Arr[117] = &CyStopDlyTar;
	CyStopDlyAct	= DataMap(DataMap::SETTER,	118,2);	Arr[118] = &CyStopDlyAct;
	CyStopDlyMod	= DataMap(DataMap::SETTER,	119,2);	Arr[119] = &CyStopDlyMod;
	CyOverlapTar	= DataMap(DataMap::GETTER,	120,2);	Arr[120] = &CyOverlapTar;
	CyOverlapAct	= DataMap(DataMap::SETTER,	121,2);	Arr[121] = &CyOverlapAct;
	CyOverlapMod	= DataMap(DataMap::SETTER,	122,2);	Arr[122] = &CyOverlapMod;
	CyStartTar		= DataMap(DataMap::GETTER,	123,2);	Arr[123] = &CyStartTar;
	CyStartAct		= DataMap(DataMap::SETTER,	124,2);	Arr[124] = &CyStartAct;
	CySucPauseTar	= DataMap(DataMap::GETTER,	125,2);	Arr[125] = &CySucPauseTar;
	CySucPauseAct	= DataMap(DataMap::SETTER,	126,2);	Arr[126] = &CySucPauseAct;
	CySucPauseMod	= DataMap(DataMap::SETTER,	127,2); Arr[127] = &CySucPauseMod;
	CyPauseTar		= DataMap(DataMap::GETTER,	128,2);	Arr[128] = &CyPauseTar;
	CyPauseAct		= DataMap(DataMap::SETTER,	129,2);	Arr[129] = &CyPauseAct;
	CyResumeTar		= DataMap(DataMap::GETTER,	130,2);	Arr[130] = &CyResumeTar;
	CyResumeAct		= DataMap(DataMap::SETTER,	131,2);	Arr[131] = &CyResumeAct;
	CySucPauseTar	= DataMap(DataMap::GETTER,	132,2);	Arr[132] = &CyPauseDlyTar;
	CySucPauseAct	= DataMap(DataMap::SETTER,	133,2);	Arr[133] = &CyPauseDlyAct;
	CySucPauseMod	= DataMap(DataMap::SETTER,	134,2); Arr[134] = &CyPauseDlyMod;


	//	Speed Settings			//////////////////////////////////////////////////////////////////////////
	MaxRPM			= DataMap(DataMap::GETTER,	140,2);	Arr[140] = &MaxRPM;
	PwrStrtDlyTar	= DataMap(DataMap::GETTER,	141,2);	Arr[141] = &PwrStrtDlyTar;
	PwrStrtDlyAct	= DataMap(DataMap::SETTER,	142,2);	Arr[142] = &PwrStrtDlyAct;
	//	EXTERNAL DEVICE SETTINGS//////////////////////////////////////////////////////////////////////////
	PrintSignalTar	= DataMap(DataMap::GETTER,	160,2);	Arr[160] = &PrintSignalTar;
	PrintSignalAct	= DataMap(DataMap::SETTER,	161,2);	Arr[161] = &PrintSignalAct;
	PrintSignalMod	= DataMap(DataMap::SETTER,	162,2);	Arr[162] = &PrintSignalMod;
	PrintSignalOnTar	= DataMap(DataMap::GETTER,	163,2);	Arr[163] = &PrintSignalOnTar;
	PrintSignalOnAct	= DataMap(DataMap::SETTER,	164,2);	Arr[164] = &PrintSignalOnAct;

	Beacon1Active	= DataMap(DataMap::GETTER,	167,1);	Arr[167] = &Beacon1Active;
	Beacon2Active	= DataMap(DataMap::GETTER,	168,1);	Arr[168] = &Beacon2Active;
	
	//	Manual IO
	ForceDI6		= DataMap(DataMap::GETTER,	200,1); Arr[200] = &ForceDI6;
	ForceDI7		= DataMap(DataMap::GETTER,	201,1); Arr[201] = &ForceDI7;
	ForceDI8		= DataMap(DataMap::GETTER,	202,1); Arr[202] = &ForceDI8;
	ForceA9			= DataMap(DataMap::GETTER,	203,1); Arr[203] = &ForceA9;
	ForceA10		= DataMap(DataMap::GETTER,	204,1); Arr[204] = &ForceA10;
	ForceA11		= DataMap(DataMap::GETTER,	205,1); Arr[205] = &ForceA11;
	ForceA12		= DataMap(DataMap::GETTER,	206,1); Arr[206] = &ForceA12;
	ForceDI6.Priority = 1;	ForceDI7.Priority = 1;	ForceDI8.Priority = 1;
	ForceA9.Priority = 1;	ForceA10.Priority = 1;	ForceA11.Priority = 1;	ForceA12.Priority = 1;
	
}