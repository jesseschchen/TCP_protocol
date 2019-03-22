#ifndef PACK
#define PACK

#include "packet.h"
#include "datagram.h"

using namespace std;

class packetMaker
{
public: 
	Packet datagramToPacket(Datagram pack);

	Datagram packetToDatagram(Packet pack);

	Datagram makeDatagram(const char* data, int length);

	packetMaker();
private: 
};



#endif