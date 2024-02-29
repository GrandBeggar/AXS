/*
 * MachineState.h
 *
 * Created: 9/24/2022 6:17:54 PM
 *  Author: kevin
 */ 


#ifndef MACHINESTATE_H_
#define MACHINESTATE_H_

class MachineState {
	friend class AXFManager;
	friend class States;
	
public:
	typedef enum {
		//	[0]	|	PowerState = 0
		POWER_OFF = 0,
		
		//	[1]	|	PowerState = 1
		POWER_READY = 1,
		
		//	[2]	|	PowerState = 2-3
		POWER_START_DELAY = 2,
		
		//	[3]	|	PowerState = 4
		IDLE = 3,
		
		//	[4]	|	Cycle Start Delay Timer Active
		CYCLE_START_DELAY = 4,
		
		//	[5]	|	Cycle Running	|	Watch for stoppage events
		CYCLE_ACTIVE = 5,
		
		//	[6]	|	Cycle waiting for pouches to clear
		CYCLE_CLEARING = 6,

		//	[7]	|	Cycle Stop Delay Timer Active
		CYCLE_STOPPING = 7,

		//	[8]	Cycle Paused	|	Only active when down stream triggered the stoppage.
		//								Deactivate Pause Bit once in this state.
		//								Watch for all clear to restart
		//								Watch for stoppage events to reset to idle.
		CYCLE_PAUSE = 8,
		
		//	[11]	Manual Operation	|	Deactivates all automatic sequences.
		MANUAL_OPERATION = 10,
		
		MACHINE_MESSAGE = 14,
		//	[15]	Machine Faulted		|	Track actual faults in separate field
		//								|	Can only get pushed to this state from
		//									External events.
		MACHINE_FAULTED = 15	//	|	Watch for events to clear, and start button to reset.

	} Status;
	
	Status status;
	int statusInt;
	
	
	bool CycleActive;
	
	void Refresh();
	
	void Reset();

	void StopCycle();
	
	void Idle();
	
	MachineState();
	
	

protected:

private:
		
	struct Lamp {
		bool RedSolid;
		bool GreenSolid;
		bool BlueSolid;
		bool BlinkEnable;
		bool RedBlink;
		bool GreenBlink;
		bool BlueBlink;
	};

	Lamp Lamps[16];

	int		CycleStartActual;
	int		CycleStartTarget;
	
	int		CycleStopActual;
	int		CycleStopTarget;
	
	bool	BlinkActive;
	int		BlinkOn;
	int		BlinkOff;
	
	void	RefreshLamp(Lamp);
	
	void	Init();
	
};	//	MACHINESTATE CLASS

#endif /* MACHINESTATE_H_ */