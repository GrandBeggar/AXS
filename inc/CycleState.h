/*
 * CycleState.h
 *
 * Created: 9/24/2022 7:01:14 PM
 *  Author: kevin
 */ 


#ifndef CYCLESTATE_H_
#define CYCLESTATE_H_

class CycleState {
	friend class AXFManager;
	friend class States;
	friend class StateHolder;
	
public:
	
	typedef enum {
		//	[0]	|	Machine is not cycling
		CYCLE_OFF = 0,
		
		//	[1]	|	Cycle Active	|	Trying to run a pouch
		CYCLE_TRY = 1,

		//	[1]	|	Cycle Active	|	Trying to run a pouch
		CYCLE_FEEDING = 2,
		
		//	[2]	|	Cycle Active	|	Handle Success
		CYCLE_SUCCESS = 3,
		
		//	[3]	|	Cycle Active	|	Handle Failure
		CYCLE_FAILURE = 4
	} Status;
	
	Status status;
	
	void Refresh(void);
	void Reset(void);

	CycleState();
	
protected:
	int RateStart;
	
private:

	void Success(int);
	void Failure(void);
	void CheckSpeed();
	void Fault();

	int CycleTryDurationTarget;
	int CycleTryPauseTarget;
	int CycleOverlapDurationTarget;
	int CycleSucPauseTarget;
	bool CyclePause;
	
	int CycleDebounceTarget;
	int CycleDebounceActual;
	
	bool PouchQtyCounted;
	bool PouchLengthCounted;
	int LastPouchStart;
	int LastPouchAcc;
	int LastPouch;
	int PouchTotal;
	int PouchCount;
	int PouchAvg;
	int PouchAvgAcc;
	int PouchMax;
	
	int RateArr[10];
	int Rate;
	int RateAcc;
	int RateCount;
	int RateLength;
		
	bool CycleUpdated;

};	//	CYCLESTATE CLASS

#endif /* CYCLESTATE_H_ */