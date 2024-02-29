/*
 * CPPFile1.cpp
 *
 * Created: 9/11/2022 1:39:20 PM
 *  Author: kevin
 */ 

#include <map>

#include "CommsManager.h"
#include "AXFManager.h"

#define MAX_PACKET_LENGTH 1000
#define LOCAL_PORT 502
#define CHECK_LINK_PERIOD 5000
#define CONNECTION_TIMEOUT 10000
#define LP_MESSAGE_INTERVAL 1000
#define HP_MESSAGE_INTERVAL 10

unsigned char packetReceived[MAX_PACKET_LENGTH];

IpAddress LocalIP = IpAddress(192, 168, 2, 200);
IpAddress NetMask = IpAddress(255, 255, 255, 0);

IpAddress RemoteIP = IpAddress(192, 168, 2, 201);

EthernetUdp Udp;

extern DataHolder Data;
extern AXFManager AXF;

void Comms::RefreshComms() {
	
	ms = Milliseconds();
	
		CheckMessages();

}

void Comms::AddMessage(DataMap * _data, int _priority) {
	
	if (_priority <= 1)
		BufferLP[_data->Address] = _data;
	if (_priority == 2)
		BufferHP[_data->Address] = _data;
	
}

void Comms::SendMessages() {
	AXF.Scan.CommsSMsST.Start();

	memset(messagesBuffer,0,1000);


	//Iterate over buffer maps
	if (sendHPTarget <= ms) {
		sendHPTarget = ms + HP_MESSAGE_INTERVAL;
		Buffer.insert(BufferHP.begin(), BufferHP.end());
		BufferHP.clear();
	}
	
	if (sendLPTarget <= ms) {
		sendLPTarget = ms + LP_MESSAGE_INTERVAL;
		Buffer.insert(BufferLP.begin(), BufferLP.end());
		BufferLP.clear();
	}
	
	for (std::map<const int, DataMap *>::iterator it=Buffer.begin();it!=Buffer.end();++it) {
		
	//	Data Types
	//	0	Read	1 Bit			Not Used
	//	1	Read	1 Byte
	//	2	Read	2 Bytes
	//	3	Read	4 Bytes
	//	4	Read	4 Bytes Float	Not Used
	
	//	Functions
	//	0	Get					Requesting a write return
	//	1	Set					Write
	//	2	Handshake Request	Request verification of value
	//	3	Handshake Return	No response required unless values do not match -- end chain?

	//	
	//	Send(0)	-> Received(0)	->	Send(1)	->	
	
	//	Format	Initiator	Function	Type	Address				Value	Terminator	Length
	//	Bit ->	@			0-Read		0-Bit	Addr * 16 + pos		0/1		!
	//			1			1			1		5					1		1			10
	
	//	Int	->	@			1-Write		2-Int	5
	
	//	
		int funcType = 0;
		
		if (it->second->GetSet == DataMap::SETTER ||
		(it->second->GetSet == DataMap::COUNTER && it->second->Local != it->second->Remote)
		)
			funcType = 1;
	
		if (it->second->DataType == 1)
			sprintf(messageBuffer, "@1%01i%05i%03i!", funcType, it->second->Address, it->second->Local);
		else if (it->second->DataType == 2)
			sprintf(messageBuffer, "@2%01i%05i%05i!", funcType, it->second->Address, it->second->Local);
		else if (it->second->DataType == 3)
			sprintf(messageBuffer, "@3%01i%05i%10i!", funcType, it->second->Address, it->second->Local);

		char newMessage[19];
		
		it->second->Sent = it->second->Local;
	
		sprintf(messagesBuffer,"%s%s",messagesBuffer,messageBuffer);
	}
	Buffer.clear();
	
	Udp.Connect(RemoteIP,LOCAL_PORT);
	Udp.PacketWrite(messagesBuffer);
	Udp.PacketSend();
	
	AXF.Scan.CommsSMsST.Stop();
}


void Comms::CheckMessages() {
	AXF.Scan.CommsCMST.Start();


	packetSize = Udp.PacketParse();
	if (packetSize > 0) {

		bytesRead = Udp.PacketRead(packetReceived, MAX_PACKET_LENGTH);
		receivedPacket = (char*)packetReceived;
		processPacket = receivedPacket;
		
		if (processPacket.size() > 0) {
			while (true) {
				lastTerminator = processPacket.find("!");
				packetLength = processPacket.size();
				if (lastTerminator > 0) {
					parseString		= processPacket.substr(0,lastTerminator);
					if (parseString.substr(0,1) == "@") {
						
						parseType		= parseString.substr(1,1);
						parseFunc		= parseString.substr(2,1);
						parseAddress	= parseString.substr(3,5);
						rcvtype			= std::stoi(parseType);
						func			= std::stoi(parseFunc);
						address			= std::stoi(parseAddress);
						
						if (rcvtype == 1)
							valLength = 3;
						else if (rcvtype == 2)
							valLength = 5;
						else if (rcvtype == 3)
							valLength = 10;						

						parseValue	= parseString.substr(8,valLength);
						if (valLength > 5)
							value			= std::stol(parseValue,0,valLength);
						else
							value	= std::stoi(parseValue);

						
					
						//	Check if the received address exists in the Data.Arr
						//		-> then set(Get) the new remote value
						if (Data.Arr.count(address) > 0) {
							packetsReceived++;
							if (Data.Arr[address]->GetSet == DataMap::GETTER)
								if (Data.Arr[address]->Local == value)
									Data.Arr[address]->Sync();
								else
									Data.Arr[address]->GS(value);
							else if (Data.Arr[address]->GetSet == DataMap::SETTER)
								Data.Arr[address]->Get(value);
							else if (Data.Arr[address]->GetSet == DataMap::COUNTER) {
								if (func == 1 || (Data.Arr[address]->Local == 0 && Data.Arr[address]->Remote == 0))
									Data.Arr[address]->GS(value);
								else
									Data.Arr[address]->Sync();
							}
						}
					}
					processPacket = processPacket.substr(lastTerminator + 1,packetLength);					
				} else
					break;				
			}
		}
	}
	AXF.Scan.CommsCMST.Stop();
}

bool Comms::CheckLink() {
	
	if (TimerCheckLink <= ms) {
		TimerCheckLink = ms + CHECK_LINK_PERIOD;

		if (EthernetMgr.PhyLinkActive())
			return true;
		else {
			return false;
		}
	}
	if (PHYConnected) return true;
	else return false;
}

bool Comms::CreateConnection() {

	if (!EthernetMgr.EthernetActive()) {
		EthernetMgr.Setup();
		EthernetMgr.NetmaskIp(NetMask);
		EthernetMgr.LocalIp(LocalIP);
		Delay_ms(100);
	}
	if (!UDPOpen)
		UDPOpen = Udp.Begin(LOCAL_PORT);
	if (EthernetMgr.EthernetActive() && UDPOpen) return true;
	else return false;
}			
	

			