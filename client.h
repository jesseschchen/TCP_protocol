#ifndef CLIENT
#define CLIENT

#include <sys/socket.h>
#include <deque>
#include "packetMaker.h"
//#include <atomic>

using namespace std;

class Client
{
public: 
	int create_socket(const char* hostname, int port_num);
	int send_message(Packet p);
	int recv_message(Datagram* message);
	int handshake();
	int footshake();
	int overflow_check(int* num, int* counter);
	int spawn_parallel(char* filename);
	int in_queue(deque<int>* q, int num);
	int clear_queue(deque<int>* q, int up);
	void print_send(Packet p);
	void print_recv(Packet p);
	void print_drop(Packet p);
	
	Client();



private:
	static int listen_thread(Client* thi, packetMaker* p_maker);
	static int send_thread(Client* thi, packetMaker* p_maker, char* filename);
	static void timer(Client* thi, bool* rst, double sec);
	static void timer2(int sec, int sockfd);
	static void timer3(double sec, bool* run, bool* set);
	static void timer4(Client* thi, bool* rst, double sec);

	bool send = 1;
	bool timeout = 0;

	int hand = 1;

	mutex mtx;

	bool end = 0;

	int final_seq = -1;

	const static int SS = 0;
	const static int CA = 1;
	int state = SS;

	int dup_count = 0; //duplicate ack count

	bool timer_reset = 0;
	bool ten_rst =  0;

	int conn_id = 0;
	int sockfd = 0;
	int ss_thresh = 10000;
	int cwnd = 512;
	int ack = 0;
	int ack_in = 12345;
	deque<int> unacked_seq;
	int seq = 12345;
	int seq_overflow_count = 0;
	int ack_in_overflow_count = 0;
	bool send_next = true;
	int window_base = 0;
	int go_back = 0;
	const static int max_ack_seq = 102400;
	const static int pack_size = 512;
	struct sockaddr_in serverAddr;
	packetMaker* p_maker;

	int max_seq = 0;
	int max_seq_overflow = 0;
};






#endif