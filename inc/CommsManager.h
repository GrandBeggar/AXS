/*
 * Ethernet.h
 *
 * Created: 9/11/2022 1:40:25 PM
 *  Author: kevin
 */ 
#include <string>
#include <map>

#include "EthernetUdp.h"
#include "DataManager.h"

#include "ClearCore.h"


#ifndef ETHERNET_H_
#define ETHERNET_H_

class Comms {
public:
	friend class AXFManager;

	void RefreshComms();
	void AddMessage(DataMap * _data, int _priority);
	void SendMessages();

	
private:
	std::map<const int, DataMap *> BufferLP;
	std::map<const int, DataMap *> BufferHP;
	std::map<const int, DataMap *> Buffer;
	
	char messagesBuffer[1000];
	char messageBuffer[18];
	
	int sendLPTarget;
	int sendHPTarget;

	bool CheckLink();
	bool CreateConnection();
	void CheckMessages();
	
	bool ConnectionReady;
	bool PHYConnected;
	bool ClientActive;
	bool UDPOpen;
	bool ActiveClient;
	
	int ms;
	
	int checkMessageTarget;
	int sendMessageTarget;
	
	int TimerCheckLink;
	
	int32_t bytesRead;
	uint16_t packetSize;
	
	// Last Incoming Packet
	std::string parseString;
	std::string parseType;
	std::string parseFunc;
	std::string parseAddress;
	std::string parseValue;
	std::string processPacket;
	std::string receivedPacket;
	int packetLength;
	int lastTerminator;
	int rcvtype;
	int func;
	int	address;
	int valLength;
	long value;
	long packetsReceived;
	
};	//	Comms Class

	
#endif /* ETHERNET_H_ */