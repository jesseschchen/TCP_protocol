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
#include <signal.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <ctime>

#include "server.h"
#include "packetMaker.h"
#include "packet.h"
#include "datagram.h"

using namespace std;

bool ending = false;

void signal_handler(int signal)
{
	ending = true;
	cout<<"exiting\n";
	exit(0);
}

Server::Server(char* save_path)
{
  	p_maker = new packetMaker();
  	this->save_path = save_path;
  	string str = "ERROR: Unresponsive client";
  	ignore_message = (char*)str.c_str();
}

void Server::print_send(Packet p)
{
	cout<<"SEND ";
	cout<<p.seq_num<< " ";
	cout<<p.ack_num<< " ";
	cout<<p.conn_id<<" ";
	cout<<"512 ";
	cout<<"10000 ";
	if(p.flags & ACK)
		cout<<"ACK ";
	if(p.flags & SYN)
		cout<<"SYN ";
	if(p.flags & FIN)
		cout<<"FIN ";
	//if(p.flags & DUP)
	//	cout<<
}

void Server::print_recv(Packet p)
{
	cout<<"RECV ";
	cout<<p.seq_num<< " ";
	cout<<p.ack_num<< " ";
	cout<<p.conn_id<<" ";
	cout<<"512 ";
	cout<<"10000 ";
	if(p.flags & ACK)
		cout<<"ACK ";
	if(p.flags & SYN)
		cout<<"SYN ";
	if(p.flags & FIN)
		cout<<"FIN ";
	cout<<endl;
}

void Server::print_drop(Packet p)
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

void Server::timer3(double sec, bool* run, bool* set)
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

void Server::timer4(Server* thi, vector<bool>* rst, double sec)
{
	vector<clock_t> start;
	int j, m;

	while(!ending)
	{
		j  = (*rst).size();
		m = start.size();
		if(m < j)
		{
			for(int k = 0; k < j - m; k++)
			{
				start.push_back(clock());
			}
		}
		for(int i = 0; i < j; i++)
		{
			if(thi->in_vector(&thi->closed_conn_id, i+1))
				continue;
			if((*rst)[i])
			{
				start[i] = clock();
				(*rst)[i] = 0;
			}
			else
			{
				if(((clock() - start[i])/(double)CLOCKS_PER_SEC) >= sec)
				{
					thi->closed_conn_id.push_back(i+1);

					int n = write(thi->file_des[i], thi->ignore_message, strlen(thi->ignore_message));
					if(n == -1)
					{
						perror("write");
					}

					close(thi->file_des[i]);

				}
			}
		}
	}
}

void Server::send4(Server* thi, Packet fin_fin)
{
	bool run = true;
	bool set = false;
	thread t3(timer3, 0.5, &run, &set);
	t3.detach();
	Datagram fin_ack;
	Packet finack;
	for(int i = 0; i < 3;)
	{
		if(thi->close_conn_ack  == fin_fin.conn_id)
		{
			run = false;
			break;
		}
		if(set)
		{
			thi->seq[fin_fin.conn_id-1] -= fin_fin.payload_length;
			thi->send_message(fin_fin, fin_fin.conn_id);
			//cout<<"resend fin\n";
			set = false;
			i++;
			if(i > 3)
			{
				break;
			}
		}
	}
	//cout<<"end send4\n";
}

int Server::in_vector(vector<int>* v, int num)
{
	for(unsigned int i = 0; i < v->size(); i++)
	{
		if((*v)[i] == num)
		{
			return 1;
		}
	}
	return 0;
}

int Server::overflow_check(int* num, int* counter)
{
	//return 0;
	if(*num > max_ack_seq)
	{
		*num = *num % max_ack_seq -1;
		if(counter != NULL)
		{
			*counter += 1;
		}
		return 1;
	}
	return 0;
}

int Server::create_socket(int port_num)
{
	signal(SIGQUIT || SIGTERM, signal_handler);


	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  	struct sockaddr_in addr;
  	addr.sin_family = AF_INET;
  	addr.sin_port = htons(port_num);     // short, network byte order
  	addr.sin_addr.s_addr = htonl(port_num);
  	//addr.sin_addr.s_addr = htonl();
  	addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
	    perror("bind");
	    return 1;
  	}

  	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	return 0;
}

int Server::send_message(Packet p, int conn_id)
{
	//cout<<"send_message: "<<conn_id<<endl;
	//cout<<"vector length: "<<clientAddr.size()<<endl;
    Datagram message = p_maker->packetToDatagram(p);
    print_send(p);

    if(ack_overflow[conn_id-1] > max_ack_overflow[conn_id-1] || (p.ack_num > max_ack[conn_id-1] && ack_overflow[conn_id-1] == max_ack_overflow[conn_id-1]))
    {
    	max_ack[conn_id-1] = p.ack_num;
    	max_ack_overflow[conn_id-1] = ack_overflow[conn_id-1];
    }
    else
    {
    	cout<<"DUP";
    }
    cout<<endl;

    if(sendto(sockfd, message.data, p.payload_length + 12, 0, (struct sockaddr*)&clientAddr[conn_id-1], client_len[conn_id-1]) == -1) 
    {
      //NEED MODIFICATION********************* to different ip address
      perror("server: sendto failed");
      return -1;
    }
    int temp = conn_id -1;
    if((p.flags & SYN) || (p.flags & FIN))
    {
      seq[temp]++;  
    }
    else
    {
      seq[temp] += p.payload_length;  
    }
    overflow_check(&(seq[temp]), NULL);
  
    return 0; 
}

int Server::recv_message(Datagram* message, struct sockaddr_in* client_addr, socklen_t* client_len)
{
    message->data = (char*)malloc(525);
    message->length = recvfrom(sockfd, message->data, 524, 0, (struct sockaddr*)client_addr, client_len);
    if((int)message->length == -1)
    {
    	free(message->data);
      	//perror("recvfrom failed");
      	return -1;
    }
    return 0;
}

int Server::handshake(struct sockaddr_in client_addr, socklen_t client_leng, Packet* conv_mess)
{
	//cerr<<"server: handshake()\n";
	char* itoa = (char*)malloc(10);
	if(save_path[strlen(save_path)-1] == '/')
		sprintf(itoa, "%d.file", conn_id_count);
	else
		sprintf(itoa, "/%d.file", conn_id_count);
	char* fullpath = (char*)malloc(strlen(save_path) + strlen(itoa));
	char* pathcpy = (char*)malloc(strlen(save_path));
	strcpy(pathcpy, save_path);
	fullpath = strcat(pathcpy, itoa);
	int out_file = open(fullpath, O_CREAT|O_WRONLY|O_TRUNC, 0600);
	if(out_file == -1)
	{
		perror("SERVER: open");
		exit(1);
	}

	//fill out packet
    Packet sync;
    sync.seq_num = 4321;
    sync.ack_num = 12346;
    sync.conn_id = conn_id_count;
    conn_id_count++;
    sync.flags = SYN|ACK;
    sync.payload_length = 0;

    //update vectors
    send_next.push_back(true);
    ack.push_back(12346);
    seq.push_back(4321);
    cwnd.push_back(512);
    ss_thresh.push_back(10000);
    file_des.push_back(out_file);
    clientAddr.push_back(client_addr);
    client_len.push_back(client_leng);
    ack_in.push_back(0);
    ack_overflow.push_back(0);
    max_ack_overflow.push_back(0);
    max_ack.push_back(0);
    tmr_rst.push_back(false);


    if(send_message(sync, sync.conn_id) == -1)
    {
        cerr<<"server: send_message() failed"<<endl;
        return -1;
    }

    bool run = true;
	bool set = false;
	thread t3(timer3, 0.5, &run, &set);
	t3.detach();

	Datagram ack1;
	Packet ack_1;
	while(true)
	{
		while(recv_message(&ack1, NULL, NULL) == -1)
		{
			if(set)
			{
				seq[sync.conn_id-1] -= sync.payload_length;
				send_message(sync, sync.conn_id);
				//cout<<"resend synack"<<endl;
				set = false;
			}
		}
		ack_1 = p_maker->datagramToPacket(ack1);
		free(ack1.data);
		if(ack_1.flags == ACK && ack_1.seq_num == ack[sync.conn_id-1] && ack_1.conn_id == sync.conn_id)
		{
			//print_recv(ack_1);
			//ack[ack_1.conn_id-1] += ack_1.payload_length;
			run = false;
			break;
		}
		print_drop(ack_1);
	}
	*conv_mess = ack_1;
	//ack[ack_1.conn_id -1] -= ack_1.payload_length;

    return 0;
}

int Server::send_ack(Packet in)
{
	Packet message;
	message.seq_num = seq[in.conn_id - 1];
	message.ack_num = ack[in.conn_id - 1];
	message.conn_id = in.conn_id;
	message.flags = ACK;
	message.payload = NULL;
	message.payload_length = 0;

	if(send_message(message, in.conn_id) == -1)
	{
		cerr<<"server:send_ack():send_message()\n";
		return -1;
	}
	//cout<<"server:ackout: "<<ack[in.conn_id-1];

	return 0;
}

int Server::listen_thread(Server* thi, packetMaker* p_maker)
{
	thread t10(timer4, thi, &thi->tmr_rst, 10);
	t10.detach();

	Datagram in_message;
	Packet conv_mess;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	short cid = 0;
	short ele = 0;
	int writ = 0;

	while(!ending)
	{
		while(thi->recv_message(&in_message, &client_addr, &client_addr_len) == -1)
		{
		}
		conv_mess = p_maker->datagramToPacket(in_message);
		//thi->print_recv(conv_mess);
		free(in_message.data);

		if(conv_mess.conn_id >= thi->conn_id_count)
		{
			thi->print_drop(conv_mess);
			//cout<<"print drop\n";
			continue;
		}

		cid = conv_mess.conn_id;
		ele = cid-1;

		if(cid > 0)
		{
			thi->tmr_rst[ele] = 1;
		}

		if(conv_mess.flags & SYN)
		{
			thi->print_recv(conv_mess);
			thi->handshake(client_addr, client_addr_len, &conv_mess);
			//SETUP NEW CONNECTION!!!!!
			//DO HANDSHAKE
			//save ip_addr
		}
		else if(conv_mess.flags & FIN)
		{
			thi->print_recv(conv_mess);
			thi->closed_conn_id.push_back(cid);

			Packet fin_ack;
			fin_ack.seq_num = thi->seq[ele];
			fin_ack.ack_num = conv_mess.seq_num + 1;
			fin_ack.conn_id = cid;
			fin_ack.flags = ACK | FIN;
			fin_ack.payload_length = 0;
			thi->ack[ele] = conv_mess.seq_num + 1;
			//thi->finished = true;

			thi->send_message(fin_ack, cid);

			/*Packet fin_fin;
			fin_fin.seq_num = thi->seq[ele];
			fin_fin.ack_num = 0;
			fin_fin.conn_id = cid;
			fin_fin.flags = FIN;
			fin_fin.payload_length = 0;

			thi->send_message(fin_fin, cid);*/

			thread s4(send4, thi, fin_ack);
			s4.detach();

			
			close(thi->file_des[ele]);
			//cout<<"closed: "<< cid<<endl;


			//CLOSE CONNECTION!!!!
			//BYEBYE
		}
		if((conv_mess.flags & SYN) == 0 && (conv_mess.flags & FIN) == 0)
		{
			cid = conv_mess.conn_id;
			ele = cid-1;

			if(thi->in_vector(&(thi->closed_conn_id), conv_mess.conn_id))
			{
				thi->close_conn_ack = conv_mess.conn_id;
				//thi->print_recv(conv_mess);
				thi->finished = true;
				//cout<<"closed conn\n";
			}

			thi->mtx.lock();
			if(conv_mess.seq_num == thi->ack[ele])
			{
				thi->print_recv(conv_mess);
				//cout<<conv_mess.seq_num << " :: "<<thi->ack[ele]<<endl;;
				thi->ack[ele] += conv_mess.payload_length;

				if(!thi->finished)
				{
					writ = write(thi->file_des[ele], conv_mess.payload, conv_mess.payload_length);
				}
				else
					writ = 0;
				if(writ == -1)
				{
					perror("server: write()");
					exit(-1);
				}
				thi->overflow_check(&(thi->ack[ele]), &(thi->ack_overflow[ele]));
				//cout<<"server: FRESH ACK OUT: "<<conv_mess.payload_length<< " : "<<thi->ack[ele] <<endl;;
			}
			else
			{
				thi->print_drop(conv_mess);
				//cout<<conv_mess.seq_num<< "::"<<thi->ack[ele]<<endl;
			}
			thi->mtx.unlock();
			if(!thi->finished)
			{
				thi->send_ack(conv_mess);
			}
			else
				thi->finished = false;

			free(conv_mess.payload);

		}
	}


    return 0;
}

int Server::send_thread(Server* thi, packetMaker* p_maker)
{
	int out = 0;

	Packet message;
	while(true)
	{
		for(int i = 0; i < thi->conn_id_count-1; i++)
		{
			thi->mtx.lock();
			if(thi->send_next[i])
			{
				//overflow(check)
				message.seq_num = thi->seq[i];
				message.ack_num = thi->ack[i];
				message.conn_id = i+1;
				message.flags = ACK;
				message.payload = NULL;
				message.payload_length = 0;

				out = thi->send_message(message, i+1);
				if(out == -1)
				{
					cerr<<"server: sending message failed\n";
					exit(-1);
				}
			}
			thi->mtx.unlock();
			//cout<<"send loop: "<<i<<endl;
		}
	}
    return 0;
}

int Server::spawn_parallel()
{
    thread listener(listen_thread, this, p_maker);
    //thread sender(send_thread, this, p_maker);


	listener.join();
	//sender.join();    

    return 0;
}


int main(int argc, char** argv)
{
    if(argc < 3)
    {
      std::cerr << "ERROR: Incorrect arguments\n";
    }
    int port_num = atoi(argv[1]);
    if(port_num < 1024 || port_num > 65535)
    {
      std::cerr << "ERROR: Invalid port number: " << port_num<< endl;
      exit(1);
    }
    char* save = (char*)malloc(strlen(argv[2]));
    save = argv[2];

    


    Server s(save);

    signal(SIGQUIT || SIGTERM, signal_handler);

    s.create_socket(port_num);
    s.spawn_parallel();

    return 0;
}