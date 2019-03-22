#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstdlib>
#include <netdb.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <sstream>

#include "packetMaker.h"
#include "packet.h"
#include "datagram.h"

using namespace std;

Packet packetMaker::datagramToPacket(Datagram pack)
{
	Packet input;
	input.seq_num = ntohl(((int*)pack.data)[0]);
	input.ack_num = ntohl(((int*)pack.data)[1]);
	input.conn_id = ntohs(((short*)pack.data)[4]);
	input.flags = ntohs(((short*)pack.data)[5]);
	input.payload_length = pack.length-12;

	/*input.seq_num = ((int*)pack.data)[0];
	input.ack_num = ((int*)pack.data)[1];
	input.conn_id = ((short*)pack.data)[4];
	input.flags = ((short*)pack.data)[5];
	input.payload_length = pack.length-12;*/


	if(input.payload_length > 0)
	{
		input.payload = (char*)malloc(input.payload_length);
		memcpy((void*)input.payload, &(((const char*)pack.data)[12]), input.payload_length);
	}
	

	return input;

}

Datagram packetMaker::packetToDatagram(Packet pack)
{
	Datagram out_packet;
	out_packet.data = (char*)malloc(pack.payload_length + 12);
	((int*)out_packet.data)[0] = htonl(pack.seq_num);
	((int*)out_packet.data)[1] = htonl(pack.ack_num);
	((short*)out_packet.data)[4] = htons(pack.conn_id);
	((short*)out_packet.data)[5] = htons(pack.flags);

	/*((int*)out_packet.data)[0] = pack.seq_num;
	((int*)out_packet.data)[1] = pack.ack_num;
	((short*)out_packet.data)[4] = pack.conn_id;
	((short*)out_packet.data)[5] = pack.flags;*/

	if(pack.payload_length > 0)
	{
		memcpy((void*)&(out_packet.data[12]), pack.payload, pack.payload_length);
	}
	//out_packet.length = htonl(pack.payload_length + 12);
	out_packet.length = pack.payload_length + 12;
	

	return out_packet;
}

Datagram packetMaker::makeDatagram(const char* data, int length)
{
	Datagram d;
	d.data = (char*)memcpy((char*)malloc(length), data, length);
	d.length = length;
	return d;
}

packetMaker::packetMaker()
{
}

int main2(int argc, char** argv)
{
	Packet pack;
	pack.seq_num = 1;
	pack.ack_num = 432432;
	pack.conn_id = 4;
	pack.flags = 4;
	string pay = "abcde";
	pack.payload = (char*)pay.c_str();
	pack.payload_length = strlen(pack.payload);
	cout<<"packpaylaod: "<<pack.payload<<endl;
	cout<<"packet.pay_len: "<<pack.payload_length<<endl;
	

	packetMaker* p = new packetMaker();
	Datagram bin_pack = p->packetToDatagram(pack);
	cout<<"bin_apack len: "<<bin_pack.length<<endl;
	Packet out = p->datagramToPacket(bin_pack);

	cout<<"seq: "<<out.seq_num<<endl;
	cout<<"ack: "<<out.ack_num<<endl;
	cout<<"conn: "<<out.conn_id<<endl;
	cout<<"flags: "<<out.flags<<endl;
	cout<<"payload: "<<out.payload<<endl;
	cout<<"whattt"<<endl;


	//cout<< bin_pack<<endl;

	//Packet out = p.binary_decode(packet);

	cout<< "fatboy"<<endl;
	return 0;
}