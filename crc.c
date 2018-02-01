/*
Caleb Edens - 822007959
Curtis Green - 422008537

Assignment #1
*/

/*///////////////////////////////////////////////////////////////////////////////////
Include Statements
*////////////////////////////////////////////////////////////////////////////////////
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "interface.h"

/*///////////////////////////////////////////////////////////////////////////////////
Function Declarations 
*////////////////////////////////////////////////////////////////////////////////////
int connect_to(const char *host, const char *port);
struct Reply process_command(const int sockfd, char* command);
void process_chatmode(const char* host, const char* port);
void *receive_chat(void *threadfd);
void *send_chat(void *threadfd);

/*///////////////////////////////////////////////////////////////////////////////////
Main
*////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) 
{
	if (argc != 3) {
		fprintf(stderr,
				"usage: enter host address and port number\n");
		exit(1);
	}

    display_title();
	
	while (1) {
	
		int sockfd = connect_to(argv[1], argv[2]);
		
		if (sockfd < 0){
			printf("Closing client");
			exit(1);
		}
	
		char command[MAX_DATA];
        get_command(command, MAX_DATA);

		struct Reply reply = process_command(sockfd, command);
		printf("got out of process_command\n");
		fflush(stdout);
		display_reply(command, reply);
		
		touppercase(command, strlen(command) - 1);
		if (strncmp(command, "JOIN", 4) == 0) {
			char buf[256];
			sprintf(buf,"%d",reply.port);
			printf("Now you are in the chatmode\n");
			process_chatmode(argv[1], buf);
		}
	
		close(sockfd);
    }

    return 0;
}

/*///////////////////////////////////////////////////////////////////////////////////
Helper Functions
*////////////////////////////////////////////////////////////////////////////////////

/*-----------------------------------------------------------------------------------
connect_to
@parameter host    host address given by command line argument
@parameter port    port given by command line argument
@return socket fildescriptor
---------------------------------------------*/
int connect_to(const char *host, const char *port)
{
	// ------------------------------------------------------------
	// GUIDE :
	// In this function, you are suppose to connect to the server.
	// After connection is established, you are ready to send or
	// receive the message to/from the server.
	// 
	// Finally, you should return the socket fildescriptor
	// so that other functions such as "process_command" can use it
	// ------------------------------------------------------------
	
	struct addrinfo hints, *res;
	int sockfd;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	int status;
	
	if ((status = getaddrinfo(host, port, &hints, &res)) != 0){
		fprintf(stderr,"Error with getaddrinfo: %s\n", gai_strerror(status));
		return -1;
	}
	
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sockfd < 0){
		perror ("Error creating socket\n");
		return -1;
	}
	
	if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0){
		perror ("Error connecting to server\n");
		return -1;
	}
	
	printf("Connected to host %s\n", host);
	fflush(stdout);
	
	return sockfd;
}


/*-----------------------------------------------------------------------------------
Reply
Send an input command to the server and return the result
@parameter sockfd   socket file descriptor to commnunicate
@parameter command  command will be sent to the server
@return    Reply
---------------------------------------------*/
struct Reply process_command(const int sockfd, char* command)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse a given command
	// and create your own message in order to communicate with
	// the server. Surely, you can use the input command without
	// any changes if your server understand it. The given command
    // will be one of the followings:
	//
	// CREATE <name>
	// DELETE <name>
	// JOIN <name>
    // LIST
	//
	// -  "<name>" is a chatroom name that you want to create, delete,
	// or join.
	// 
	// - CREATE/DELETE/JOIN and "<name>" are separated by one space.
	// ------------------------------------------------------------
	
	struct Reply reply;
	reply.status = FAILURE_NOT_EXISTS;
	reply.num_member = 10;
	reply.port = 3000;
	sprintf(reply.list_room, "default room,");
	
	// ------------------------------------------------------------
	// GUIDE 2:
	// After you create the message, you need to send it to the
	// server and receive a result from the server.
	// ------------------------------------------------------------

	int data[3];
	char list_room[MAX_DATA];
	memset(list_room,'\0',MAX_DATA);
	
	touppercase(command, strlen(command) - 1);
	
	//	Send/receive command & reply
	send(sockfd, command, MAX_DATA, 0);
	recv(sockfd, data, 3*sizeof(int), 0);
	recv(sockfd, list_room, MAX_DATA,0);

	printf("status = %d, num_members = %d, port = %d, list = %s\n",data[0], data[1], data[2], list_room);

	// Populate reply with server data
	reply.status = (Status)data[0];
	reply.num_member = data[1];
	reply.port = data[2];
	sprintf(reply.list_room, list_room);

	// ------------------------------------------------------------
	// GUIDE 3:
	// Then, you should create a variable of Reply structure
	// provided by the interface and initialize it according to
	// the result.
	//
	// For example, if a given command is "JOIN room1"
	// and the server successfully created the chatroom,
	// the server will reply a message including information about
	// success/failure, the number of members and port number.
	// By using this information, you should set the Reply variable.
	// the variable will be set as following:
	//
	// Reply reply;
	// reply.status = SUCCESS;
	// reply.num_member = number;
	// reply.port = port;
	// 
	// "number" and "port" variables are just an integer variable
	// and can be initialized using the message fomr the server.
	//
	// For another example, if a given command is "CREATE room1"
	// and the server failed to create the chatroom becuase it
	// already exists, the Reply varible will be set as following:
	//
	// Reply reply;
	// reply.status = FAILURE_ALREADY_EXISTS;
    // 
    // For the "LIST" command,
    // You are suppose to copy the list of chatroom to the list_room
    // variable. Each room name should be seperated by comma ','.
    // For example, if given command is "LIST", the Reply variable
    // will be set as following.
    //
    // Reply reply;
    // reply.status = SUCCESS;
    // strcpy(reply.list_room, list);
    // 
    // "list" is a string that contains a list of chat rooms such 
    // as "r1,r2,r3,"
	// ------------------------------------------------------------

	return reply;
}


/*-----------------------------------------------------------------------------------
Process_chatmode
Get into the chat mode
@parameter host     host address
@parameter port     port
---------------------------------------------*/
void process_chatmode(const char* host, const char* port)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In order to join the chatroom, you are supposed to connect
	// to the server using host and port.
	// You may re-use the function "connect_to".
	// ------------------------------------------------------------

	int sockfd = connect_to(host, port);
	
	// ------------------------------------------------------------
	// GUIDE 2:
	// Once the client have been connected to the server, we need
	// to get a message from the user and send it to server.
	// At the same time, the client should wait for a message from
	// the server.
	// ------------------------------------------------------------
	
	// Create Receiving thread and sending thread
	pthread_t r_thread, s_thread;
	int thread;

	thread = pthread_create(&r_thread, NULL, send_chat, (void *)sockfd);
	if (thread){
		printf("Client send chat thread failed to create");
	}
	thread = pthread_create(&r_thread, NULL, receive_chat, (void *)sockfd);
	if (thread){
		printf("Client receive chat thread failed to create");
	}

	pthread_exit(NULL);

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    // 1. To get a message from a user, you should use a function
    // "void get_message(char*, int);" in the interface.h file
    // 
    // 2. To print the messages from other members, you should use
    // the function "void display_message(char*)" in the interface.h
    //
    // 3. Once a user entered to one of chatrooms, there is no way
    //    to command mode where the user  enter other commands
    //    such as CREATE,DELETE,LIST.
    //    Don't have to worry about this situation, and you can 
    //    terminate the client program by pressing CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

/*===========================================================
  Receiving chat thread
  =============================================================*/
void *receive_chat(void *threadfd){
	char buff[MAX_DATA];
	memset(buff,'\0',MAX_DATA);
	long sockfd = (long) threadfd;

	while(1){
		recv(sockfd, buff, sizeof buff, 0);
		display_message(buff);
	}
	
}

/*===========================================================
  Sending chat thread
  =============================================================*/
void *send_chat(void *threadfd){
	char buff[MAX_DATA];
	memset(buff,'\0',MAX_DATA);
	long sockfd = (long) threadfd;
	while(1){
		get_message(buff, sizeof buff);
		send(sockfd, buff, sizeof buff, 0);
	}
}