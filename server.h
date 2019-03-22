#ifndef SERVER
#define SERVER

#include <sys/socket.h>
#include "packetMaker.h"
//#include <atomic>

using namespace std;

class Server
{
public: 
	int create_socket(int port_num);
	int send_message(Packet p, int conn_id);
	int recv_message(Datagram* message, struct sockaddr_in* client_addr, socklen_t* client_len);
	int handshake(struct sockaddr_in client_addr, socklen_t client_len, Packet* conv_mess);
	int in_vector(vector<int>* v , int num);

	int send_ack(Packet in);



	int spawn_parallel();

	int write_to_file(Packet p);

	int handle_connection();

	int overflow_check(int* num, int* counter);

	void timer(double sec);

	void print_send(Packet p);
	void print_recv(Packet p);
	void print_drop(Packet p);

	Server(char* save_path);



private:
	static int listen_thread(Server* thi, packetMaker* p_maker);
	static int send_thread(Server* thi, packetMaker* p_maker);
	static void send4(Server* thi, Packet fin_fin);
	static void timer3(double sec, bool* run, bool* set);
	static void timer4(Server* thi, vector<bool>* rst, double sec);


	bool send = 1;
	bool finished = 0;

	mutex mtx;

	int conn_id_count = 1;
	int sockfd = 0;
	vector<int> ss_thresh; //10000
	vector<int> cwnd; //512
	//atomic<int> ac1; //0
	//atomic<int> seq1; //12345
	//atomic<bool> send_next1; //true
	vector<int> ack; //
	vector<int> seq; //54321
	vector<bool> send_next;
	vector<int> file_des;
	int window_base = 0;
	const static int max_ack_seq = 102400;
	const int pack_size = 512;
	vector<struct sockaddr_in> clientAddr;
	vector<socklen_t> client_len;
	vector<int> ack_in;
	vector<int> closed_conn_id;
	int close_conn_ack = -1;

	char* save_path;
	char* fullpath;
	packetMaker* p_maker;

	vector<int> ack_overflow;
	vector<int> max_ack_overflow;
	vector<int> max_ack;

	vector<bool> tmr_rst;

	char* ignore_message;
};






#endif