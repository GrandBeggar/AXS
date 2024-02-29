/*
 * PowerState.h
 *
 * Created: 9/24/2022 5:51:40 PM
 *  Author: kevin
 */ 


#ifndef POWERSTATE_H_
#define POWERSTATE_H_

class PowerState {
	friend class AXFManager;
	friend class States;
	
public:
	typedef enum {
		//	[0]	|	Control Power is off
		OFF = 0,
	
		//	[1]	|	Estop is Clear
		READY,
	
		//	[2]	|	Control Power On ->	Delay Timer Active
		START_DELAY,
	
		//	[3]	|	Control Power Delay Done
		ON
	} Status;
	
	Status status;	//	PowerStates Pointer	//
	int statusInt;
	
	void Refresh();
	
	void Reset();

	PowerState();

private:
	//	Power Start Time		DataRegister
	int		PowerStart;
	//	Power Target Time		DataRegister
	int		PowerStartTarget;
	//	Actual MS is the HB		DataRegister
	int		ms;
	
};	//	POWERSTATE CLASS

#endif /* POWERSTATE_H_ */