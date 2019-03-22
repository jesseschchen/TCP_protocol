#ifndef PACKET
#define PACKET

const int ACK = 4;
const int SYN = 2;
const int FIN = 1;
const int HEADER_SIZE = 12;

struct Packet
{
	int seq_num = 0;
	int ack_num = 0;
	short conn_id = 0;
	short flags = 0;
	char* payload = NULL;
	int payload_length = 0;
};

#endif