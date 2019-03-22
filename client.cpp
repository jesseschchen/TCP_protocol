#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>

#include "client.h"
#include "packetMaker.h"
#include "packet.h"
#include "datagram.h"

using namespace std;



Client::Client()
{
	p_maker = new packetMaker();
}

void Client::print_send(Packet p)
{
	cout<<"SEND ";
	cout<<p.seq_num<< " ";
	cout<<p.ack_num<< " ";
	cout<<p.conn_id<<" ";
	cout<<cwnd<<" ";
	cout<<ss_thresh<<" ";
	if(p.flags & ACK)
		cout<<"ACK ";
	if(p.flags & SYN)
		cout<<"SYN ";
	if(p.flags & FIN)
		cout<<"FIN ";
}

void Client::print_recv(Packet p)
{
	cout<<"RECV ";
	cout<<p.seq_num<< " ";
	cout<<p.ack_num<< " ";
	cout<<p.conn_id<<" ";
	cout<<cwnd<<" ";
	cout<<ss_thresh<<" ";
	if(p.flags & ACK)
		cout<<"ACK ";
	if(p.flags & SYN)
		cout<<"SYN ";
	if(p.flags & FIN)
		cout<<"FIN ";
	cout<<endl;
}

void Client::print_drop(Packet p)
{
	cout<<"DROP ";
	cout<<p.seq_num<< " ";
	cout<<p.ack_num<< " ";
	cout<<p.conn_id<<" ";
	if(p.flags & ACK)
		cout<<"ACK ";
	if(p.flags & SYN)
		cout<<"SYN ";
	if(p.flags & FIN)
		cout<<"FIN ";
	cout<<endl;
}

int Client::in_queue(deque<int>* q, int num)
{
	int l = q->size();
	for(int i = l-1; i >= 0; i--)
	{
		if((*q)[i] == num)
		{
			return i;
		}
	}
	return -1;
}

int Client::clear_queue(deque<int>* q, int up)
{
	for(int i = 0; i <= up; i++)
	{
		q->pop_front();
	}
	return up;
}

int Client::overflow_check(int* num, int* counter)
{
	//return 0;
	if(*num > max_ack_seq)
	{
		*num = *num % max_ack_seq - 1;
		if(counter != NULL)
		{
			*counter += 1;
		}
		return 1;
	}	
	return 0;
}

void Client::timer(Client* thi, bool* rst, double sec)
{
	clock_t start_time = clock();
	while(!thi->end)
	{
		if(*rst)
		{
			start_time = clock();
			*rst = 0;
		}
		else
		{
			if(((clock() - start_time)/(double)CLOCKS_PER_SEC) >= sec)
			{
				thi->mtx.lock();
				//cerr<<"TIMEOUT!!!: " << thi->seq << "."<<thi->seq_overflow_count<< " -> " << thi->ack_in<< "." << 
				//	thi->ack_in_overflow_count<<endl;;
				thi->ss_thresh = thi->cwnd/2;
				thi->cwnd = pack_size;
				thi->go_back = (thi->seq + 511 + thi->ack_in)/(pack_size);  //wait what???
				thi->seq = thi->ack_in;
				thi->seq_overflow_count = thi->ack_in_overflow_count;
				thi->unacked_seq.clear();
				thi->state = SS;
				//TIMEOUT FUCKERS
				thi->overflow_check(&(thi->go_back), NULL);
				thi->timeout = true;

				thi->mtx.unlock();

				start_time = clock();
			}
		}
	}
}

void Client::timer2(int sec, int sockfd)
{
	sleep(sec);
	close(sockfd);
	exit(0);
}

void Client::timer3(double sec, bool* run, bool* set)
{
	clock_t start = clock();
	while(*run)
	{
		if(((clock() - start)/(double)CLOCKS_PER_SEC) >= sec)
		{
			*set = true;
			start = clock();
		}
	}
}

void Client::timer4(Client* thi, bool* rst, double sec)
{
	clock_t start = clock();
	while(!thi->end)
	{
		if(*rst)
		{
			start = clock();
			*rst = 0;
		}
		else
		{
			if(((clock() - start)/(double)CLOCKS_PER_SEC) >= sec)
			{
				close(thi->sockfd);
				exit(2);
			}
		}
	}
}

int Client::create_socket(const char* hostname, int port_num)
{
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  	struct sockaddr_in addr;
  	addr.sin_family = AF_INET;
  	addr.sin_port = htons(0);     // short, network byte order
  	//addr.sin_addr.s_addr = htonl(0);
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
	    perror("bind");
	    return 1;
  	}

  	hostent* server_ip = gethostbyname(hostname);
  	if(server_ip == NULL)
  	{
	  	cerr<<"ERROR: unable to get ip address"<<endl;
	  	exit(1);
  	}

  	serverAddr.sin_family = AF_INET;
  	serverAddr.sin_port = htons(port_num);     // short, network byte order
  	memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  	memcpy(&(serverAddr.sin_addr), server_ip->h_addr_list[0], server_ip->h_length);

  	fcntl(sockfd, F_SETFL, O_NONBLOCK);


	return 0;
}

int Client::send_message(Packet p)
{
	//if(sendto(sockfd, (const void*)message, ))
	Datagram message = p_maker->packetToDatagram(p);
	print_send(p);
	if(seq_overflow_count > max_seq_overflow || (p.seq_num > max_seq && seq_overflow_count == max_seq_overflow))
	{
		max_seq = p.seq_num;
		max_seq_overflow = seq_overflow_count;
	}
	else
	{
		cout<<"DUP";
		//cout<<"\n max: "<<max_seq<<"."<< max_seq_overflow << " > " <<p.seq_num <<"."<<seq_overflow_count<<endl;
	}
	cout<<endl;


	if(sendto(sockfd, message.data, p.payload_length + 12, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
	{
		perror("client: sendto failed1");
		//cout<< ntohl(message.length)<<endl;
		return -1;
	}
	if((p.flags & SYN) || (p.flags & FIN))
	{
		seq++;
		overflow_check(&seq, &seq_overflow_count);
	}
	else
	{
		//cout<<"out_seq: "<<seq<<endl;
		//cout<<"seq += "<<p.payload_length<<endl;
		seq += p.payload_length;
		overflow_check(&seq, &seq_overflow_count);
	}
	unacked_seq.push_back(seq);
	free(message.data);	
	
	return 0; 
}

int Client::recv_message(Datagram* message)
{
	message->data = (char*)malloc(525);
	message->length = recvfrom(sockfd, message->data, 524, 0, 0, 0);
	if((int)message->length == -1)
	{
		free(message->data);
		//perror("recvfrom failed");
		return -1;
	}
	ten_rst = 1;
	return 0;
}

int Client::handshake()
{
	thread t10(timer4, this, &ten_rst, 10);
	t10.detach();

	Packet sync;
	sync.seq_num = seq;
	sync.ack_num = 0;
	sync.conn_id = 0;
	sync.flags = SYN;
	sync.payload_length = 0;

	send_message(sync);

	bool run = true;
	bool set = false;
	thread t3(timer3, 0.5, &run, &set);
	t3.detach();

	Datagram synAck;
	Packet syn_ack;
	while(true)
	{
		while(recv_message(&synAck) == -1)
		{
			//cout<<"set: " << set<<endl;;
			if(set)
			{
				seq -= 1;
				send_message(sync);
				set = false;
			}
		}
		syn_ack = p_maker->datagramToPacket(synAck);
		print_recv(syn_ack);
		free(synAck.data);
		if(syn_ack.flags == (SYN | ACK)	 && syn_ack.ack_num == 12346 && syn_ack.seq_num == 4321)
		{
			ack = 4321;
			run = false;
			cwnd += 512;
			break;
		}
		else
		{
			print_drop(syn_ack);
		}

	}
	unacked_seq.clear();
	//unacked_seq.push_back(12346);

	conn_id = syn_ack.conn_id; 
	ack += 1;
	return 0;
}

int Client::footshake()
{
	Packet fin;
	fin.seq_num = seq;
	fin.ack_num = 0;
	fin.conn_id = conn_id;
	fin.flags = FIN;
	fin.payload_length = 0;

	if(send_message(fin) == -1)
	{
		cerr << "send_message() failed" << endl;
		return -1;
	}
	//send fin#1

	bool run = true;
	bool set = false;
	thread t1(timer3, 0.5, &run, &set);
	t1.detach();


	Datagram fin_ack;
	Packet finack;
	while(true)
	{
		while(recv_message(&fin_ack) == -1) //wait for ack
		{
			if(set)
			{
				seq = fin.seq_num;
				send_message(fin);
				set = false;
			}
		}
		finack = p_maker->datagramToPacket(fin_ack);
		free(fin_ack.data);
		print_recv(finack);
		//cout<<"recv: "<<finack.seq_num<<", "<< finack.ack_num<<endl;
		//cout<<"shld: "<<500 << ", "<< seq<<endl;
		if(finack.flags == (ACK|FIN) && finack.ack_num == seq && finack.conn_id == conn_id)
		{
			run = false;
			break;
		}
		print_drop(finack);
	}

	thread t(timer2, 2, sockfd);
	t.detach();

	Datagram in_message;
	Packet conv;
	Packet out_message;
	out_message.seq_num = seq;
	out_message.ack_num = finack.seq_num + 1;
	out_message.conn_id = conn_id;
	out_message.flags = ACK;
	out_message.payload_length = 0;
	send_message(out_message);

	while(true)
	{
		while(recv_message(&in_message) == -1)
		{
		}
		conv = p_maker->datagramToPacket(in_message);
		print_recv(conv);
		free(in_message.data);
		if((conv.flags == (FIN|ACK) || conv.flags == FIN) && conv.conn_id == conn_id)
		{
			out_message.seq_num = seq;
			out_message.ack_num = conv.seq_num + 1;
			send_message(out_message);
		}
		else
		{
			print_drop(conv);
		}
	}
}

int Client::listen_thread(Client* thi, packetMaker* p_maker)
{
	Datagram in_message;
	Packet conv_mess;
	while(true)
	{
		while(thi->recv_message(&in_message) == -1)
		{
		}
		
		conv_mess = p_maker->datagramToPacket(in_message);
		bool in_q = 0;
		free(in_message.data);
		thi->mtx.lock();
		int que_pos = thi->in_queue(&(thi->unacked_seq), conv_mess.ack_num);
		if(que_pos != -1 || conv_mess.ack_num == thi->final_seq)
		{
			thi->print_recv(conv_mess);
			in_q = true;
			thi->clear_queue(&(thi->unacked_seq), que_pos);
			//received a valid ack
			//cout<<"VALID ACK: "<<conv_mess.ack_num<<endl;
			thi->timer_reset = 1;
			thi->dup_count = 0;
			thi->ack = conv_mess.seq_num + conv_mess.payload_length;
			thi->overflow_check(&(thi->ack), NULL);

			if(thi->state == CA)// && thi->cwnd < 85000)
			{
				thi->cwnd += (512*512) / thi->cwnd;
			}
			else if(thi->state == SS)
			{
				if(thi->cwnd < thi->ss_thresh)
				{
					thi->cwnd += 512;
				}
				else
				{
					thi->state = CA;
				}
				//cout<<"cwnd modified:::****SS\n";
			}
		}
		else
		{
			if(conv_mess.ack_num != thi->ack_in)
				thi->print_drop(conv_mess);
			//cout<<"finalseq: "<< thi->final_seq<<end;
			//cout<<"WRONG ACK: "<<conv_mess.ack_num<< " -> "<<thi->ack_in<<endl;
		}


		if(conv_mess.ack_num == thi->ack_in)
		{
			thi->print_drop(conv_mess);
			thi->dup_count++;
			if(thi->dup_count >= 3)
			{
				//TRIPLE ACK
				//seq
			}
		}
		if(conv_mess.ack_num < thi->ack_in && in_q)
		{
			thi->ack_in_overflow_count += 1;
		}
		thi->ack_in = conv_mess.ack_num;
		if(thi->ack_in == thi->final_seq)
		{
			thi->end = true;
			thi->mtx.unlock();
			//cout<<"final: "<<thi->final_seq<<endl;
			break;
		}

		if(thi->seq - conv_mess.ack_num > thi->cwnd)
		{
			thi->send_next = false;
		}
		else
		{
			thi->send_next = true;
		}
		thi->mtx.unlock();

		free(conv_mess.payload);
	}
	//does the listening
	return 0;
}

int Client::send_thread(Client* thi, packetMaker* p_maker, char* filename)
{
	
	int in_file = open(filename, O_RDONLY);
	if(in_file == -1)
	{
		cerr<< "client: open failed\n";
		exit(1);
	}

	FILE* in_stream = fdopen(in_file, "r");

	char* buf = (char*)malloc(513);  //BUFFER SIZE = window size
	int in = 0;
	int out = 0;
	int eof = 0;
	int bytes_read =0;
	Packet message;
	int temp_win = 0;


	for(int i = 0; !eof || !thi->end; i++)
	{
		temp_win = (thi->cwnd/thi->pack_size +1) * thi->pack_size;
		buf = (char*)malloc(temp_win);
		in = fread(buf, 1, temp_win, in_stream);   //MODIFY BUFFER SIZE = window size

		if(in == -1)
		{
			cerr<<"fread failed\n";
			exit(1);
		}
		else if(in < temp_win)
		{
			eof = 1;
			//cout<<"eof!!!: in: "<<in<< " cwnd: "<<temp_win<<endl;
			//break;
		}


		bytes_read += in;
		//cout<<"total bytes_read: "<<bytes_read<<endl;
		//cout<< thi->conn_id<< ": " << thi->state<< ": CLIENT: window size: "<<thi->cwnd<<endl;
		int sub_packet_num;
		if(in%thi->pack_size == 0)
			sub_packet_num = in/thi->pack_size;
		else
			sub_packet_num = in/thi->pack_size + 1;


		for(int j = 0; j < sub_packet_num || thi->timeout; j++) ////**************************************pseudo code
		{

			thi->mtx.lock();

			if(thi->timeout)
			{
				int bytenum = thi->seq + max_ack_seq * thi->seq_overflow_count - 12346 + thi->seq_overflow_count;
				fseek(in_stream, bytenum, SEEK_SET);
				//cout<<"FSEEK FILE\n";
				thi->timeout = false;
				thi->unacked_seq.clear();
				thi->mtx.unlock();
				eof = 0;
				break;
			}
			//fill out packet information
			// seq, ack, conn_id, flags, etc

			//if(thi->seq - thi->ack_in <= thi->cwnd || 
			//	(thi->seq - thi->ack_in < 0 && thi->seq + max_ack_seq - thi->ack_in <= thi->cwnd))//thi->send_next)
			if(thi->unacked_seq.size() * pack_size <= (unsigned int)thi->cwnd)
			{

				message.seq_num = thi->seq;
				message.ack_num = 0;//thi->ack;
				message.flags = 0;
				if(thi->hand)
				{
					message.ack_num = thi->ack;
					message.flags = ACK;
					thi->hand = 0;
				}
				message.conn_id = thi->conn_id;
				

				message.payload = &(buf[j*thi->pack_size]);

				if(in - j*thi->pack_size < thi->pack_size)
				{
					message.payload_length = in%thi->pack_size;
				}
				else
				{
					message.payload_length = thi->pack_size;
				}



				out = thi->send_message(message);
				if(out == -1)
				{
					cerr<<"sending message fucked up\n";
					//exit(-1);
				}

				//cout<<"client:conn:"<<thi->conn_id<<endl;;
			}
			else
			{
				j--;
				//cout<<thi->unacked_seq.size() * pack_size << " > " << thi->cwnd<<endl;
			}
			
			thi->mtx.unlock();
		}
		if(eof)
		{
			thi->final_seq = thi->seq;
			//cout<<"final_seq: "<<thi->final_seq<<endl;
		}
		

		free(buf);
	}
	//thi->final_seq = thi->seq;
	//cout<<"final_seq: "<<thi->final_seq<<endl;
	thi->unacked_seq.push_back(thi->final_seq);
	
	return 0;
}

int Client::spawn_parallel(char* filename)
{

	//TODO spawn parallel threads
	thread t(timer, this, &timer_reset, 0.5);
	thread listener(listen_thread, this, p_maker);
	thread sender(send_thread, this, p_maker, filename);

	t.join();
	listener.join();
	sender.join();


	return 0;
}


int main(int argc, char** argv)
{
	if(argc < 4)
	{
		cerr << "ERROR: Incorrect arguments\n";
	}
	int port_num = atoi(argv[2]);
  	if(port_num < 1024 || port_num > 65535)
  	{
  		cerr << "ERROR: Invalid port number: " << port_num << std::endl;
  		exit(1);
  	}
	char* host_ip = (char*)malloc(strlen(argv[1]));
	host_ip = argv[1];
	char* file_name = (char*)malloc(strlen(argv[3]));
	file_name = argv[3];
  	const char* ip = host_ip;

	Client c;

	c.create_socket(ip, port_num);
	c.handshake();
	c.spawn_parallel(file_name);
	c.footshake();
	return 0;
}

