/*
 * AXFManager.h
 *
 * Created: 9/23/2022 6:47:14 PM
 *  Author: kevin
 */ 

#ifndef AXFMANAGER_H_
#define AXFMANAGER_H_

#include <stdint.h>
#include <sam.h>


class AXFManager {
public:

	int curMS;
	int nvmMS;
	uint8_t ReadData[400];
	uint8_t * byteAddress;
	int16_t * address;
	int32_t * address32;
	//	int8_t m_nvmPageCache[512]
	//	NVM_LOCATION_TO_INDEX(loc) ((loc) + 32) ** I think this just adds 32 to whatever number is entered
	//	1 Byte
	//	nvmLocation = enum-> int
	
	//	array of 512 int8_t
	//	m_nvmPageCache[NVM_LOCATION_TO_INDEX(nvmLocation)] = newValue;
	
	//	2 Bytes
	//  int16_t *address = reinterpret_cast<int16_t *>(byteAddress);

    //	Put the new desired value into the page cache.
    //	address[0] = newValue;

	
	
	uint8_t WriteData[400];
	int returnValue;

	AXFManager();
	void Init();
	void Refresh();
	
	struct Avg {
		int Cur;
		int Max;
		int Counts;
		int Average;
		int Total;
		int AvgAcc;
		
		void Add(int _cur);
		void Clear();
		int GetAvg();
	};
	
	struct Timer {
		long int Target;
		int Length;
		bool Active;
		
		void Start(int _length);
		void Stop();
		bool Done();
		bool Cycle(int _length);
	};
	
	struct ScanObj {
		long int StartTime;
		int Length;
		int Max;
		int Scans;
		int Total;
		int Average;
		int AvgAcc;
		
		void Start();
		void Stop();
	};
	
	class ScanHolder {
		public:
		ScanObj Manager;
		ScanObj CommsST;
		ScanObj CommsCMST;
		ScanObj CommsSMsST;
		ScanObj IOST;
		ScanObj AlgorithmsST;
		ScanObj StatesST;
	};
	
	ScanHolder Scan;

    bool m_cacheInitialized = false;
    // Page cache is a byte array.
    int8_t m_nvmPageCache[NVMCTRL_PAGE_SIZE];
    // The page cache gets written to NVM as 32-bit pieces
    int32_t *m_nvmPageCache32;

    void PopulateCache();	
	
private:

	void Algorithms();
	void ProcessMotors();

	bool AXFReady;
	bool ConnectionReady;
	bool NvmDone;
	bool NvmWritten;
	
	int sendMessageTargetLP;
	int sendMessageTargetHP;
	
	float machineMaxSpeed;
	float machineSpeedTarget;
	float machineSpeedActual;
	float machineDistancePerSecond;
	
	float machineRPM;
	float machineIfRPM;
	float machineIfDistancePerSecond;
	float machineIFSpeedActual;
	
	float cycleOverlapTarget;
	float cycleOverlapDuration;
	
	float cycleTryTarget;
	float cycleTryDuration;
	
	float cycleSuccessPauseTarget;
	float cycleSuccessPauseDuration;
	
	float cyclePauseDelayTarget;
	float cyclePauseDelayDuration;
	
	float cycleStopTarget;
	float cycleStopDuration;
	
	float printSignalTarget;
	float printSignalDuration;
};

#endif /* AXFMANAGER_H_ */