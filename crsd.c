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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interface.h"

/*///////////////////////////////////////////////////////////////////////////////////
Function Declarations 
*////////////////////////////////////////////////////////////////////////////////////
int passiveTCPsock(const char * service, int backlog);
void *handle_request(void * fd);
int errexit(const char *format, ...);

/*///////////////////////////////////////////////////////////////////////////////////
Data Structures 
*////////////////////////////////////////////////////////////////////////////////////
struct ServerData{
   int fd; 
   char names[20][MAX_DATA];
   int port[20];
   int members[20];
};

/*///////////////////////////////////////////////////////////////////////////////////
Main
*////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  char * service; /* service name or port number */
  int    m_sock, s_sock;      /* master and slave socket     */

  //Setting up the server data struct that holds all information on rooms
  ServerData data;
  for(int i = 0; i < 20; i++){
    memset(data.names[i],'\0',MAX_DATA);
    data.port[i] = -1; //-1 as an invalid address placeholder
  }
  
  
 	service = argv[1];
 	printf("Server starting on port %s...",service);
  m_sock = passiveTCPsock(service, 32);
  pthread_t th; pthread_attr_t ta;
	pthread_attr_init(&ta);
  pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
  printf("Success!\n");
  for (;;) {
   	//s_sock = accept(m_sock,(struct sockaddr*)&fsin, sizeof(fsin));
   	s_sock = accept(m_sock,NULL,NULL);
   	if (s_sock < 0){
   		if (errno == EINTR) continue;
     		else errexit("accept failed: %s\n", strerror(errno));
   	}
    data.fd = s_sock;
   	pthread_create(&th, &ta, handle_request, (void*)&data);
  }
}

/*///////////////////////////////////////////////////////////////////////////////////
Helper Functions
*////////////////////////////////////////////////////////////////////////////////////


/*-----------------------------------------------------------------------------------
passiveTCPsock
This fucntion for handling the socket code is mostly taken from the lecture 
slides and adapted to work in our environment. 
---------------------------------------------*/
int passiveTCPsock(const char * service, int backlog) {
  
  struct sockaddr_in sin;          /* Internet endpoint address */
  struct servent * pse;
 	memset(&sin, 0, sizeof(sin));    /* Zero out address */
  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  
  /* Map service name to port number */
  if (pse = getservbyname(service, "tcp") )
   	sin.sin_port = pse->s_port;
  else if ((sin.sin_port = htons((unsigned short)atoi(service))) == 0)
    errexit("can’t get <%s> service entry\n", service);

  /* Allocate socket */
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) errexit("can’t create socket: %s\n", strerror(errno));

  /* Bind the socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    errexit("can’t bind to …\n");
  
  /* Listen on socket */
  if (listen(s, backlog) < 0)
    errexit("can’t listen on …\n");
  
  return s;
}

/*-----------------------------------------------------------------------------------
handle_request
This is our one stop shop for main command inputs to the server. This is where we 
tokenize the data to determine the command type, execute the various commands,
and update the serverData struct that we passed in as a pointer. 
---------------------------------------------*/
void *handle_request(void * inputData){
  //exctract the file descriptor from the serverData struct
  ServerData* servData = (ServerData*)inputData;
  int fd = servData->fd;

  //this is the array that will be sent back to the client at the end of the request
  //send [0] = enum, [1] = # members, [3] = port #, list_room = list function result
  int data[3] = {0, 15, 3005};
  char list_room[MAX_DATA];
  memset(list_room,'\0',MAX_DATA);

  fflush(stdout);

	// Get command
	char comm[MAX_DATA];
	recv(fd, comm, MAX_DATA,0);
	
	//Tokenize the input 
	char *text[2];
	char *token;
	int i = 0;
	
	token = strtok(comm, " ");
	text[0] = token;
	while(token != NULL){
		i++;
		token = strtok (NULL, " ");
		text[i] = token;
	}    

  /*===========================================================
  Join
  =============================================================*/
	if (strncmp(comm, "JOIN", 4) == 0) {
		
		//Search list of rooms
    bool exists = false; 
    for(int i = 0; i < 20; i++){
      if (strncmp(text[1],servData->names[i],sizeof(text[1])) == 0){
        exists = true; 

        //Update server data
        servData->members[i] = 1;

        //Update client data
        data[0] = 0;
        data[1] = servData->members[i];
        data[2] = servData->port[i];

        //TODO Make the join happen; 
        printf("Joining room %s\n", text[1]);
        break;
      }
    }

    //Room not found case
    if(!exists){
      data[0] = FAILURE_NOT_EXISTS;
    }
	} 
	
  /*===========================================================
  List
  =============================================================*/
	else if (strncmp(comm, "LIST", 4) == 0) {
		//loop though names list to get all of the room titles
    int index = 0;
    for(int i = 0; i < 20; i++){
      int j = 0;
      //this loop runs through the individual chars in the titles
      while(isalpha(servData->names[i][j]) || isdigit(servData->names[i][j])){
        list_room[index] = servData->names[i][j];
        index++;
        j++;
      }
      
      //this decides where the commas should go
      if(index != 0 && list_room[index-1] != ',' ){
        list_room[index] = ',';
        index++;
      }
    }
    printf("Listing rooms\n");
	}

	/*===========================================================
  Create
  =============================================================*/
	else if (strncmp(comm, "CREATE", 6) == 0) {
		printf("Creating room %s\n", text[1]);
		
    //Check if room exists		
    bool exists = false; 
    for(int i = 0; i < 20; i++){
      if (strncmp(text[1],servData->names[i],sizeof(text[1])) == 0){
        exists = true; 
        data[0] = 1;
      }
    }

    //Create room
    if(!exists){
      for(int i = 0; i < 20; i++){
        if(servData->port[i] == -1){
          strcpy(servData->names[i],text[1]);
          //TODO: update port data to accurate info
          servData->port[i] = 171717;
          servData->members[i] = 0;
          data[0] = 0;
          data[1] = 0;
          data[2] = servData->port[i];
          break;
        }
      }
    }
  }
	
  /*===========================================================
  Delete
  =============================================================*/
  //data[0] = success/fail
	else if (strncmp(comm, "DELETE", 6) == 0) {
		printf("Deleting room %s\n", text[1]);

    //Search list of rooms
    bool exists = false; 
    for(int i = 0; i < 20; i++){
      if (strncmp(text[1],servData->names[i],sizeof(text[1])) == 0){
        exists = true; 

        //Update server data
        //TODO: actually delete something
        servData->members[i] = 0;
        servData->port[i] = -1;
        memset(servData->names[i],'\0',MAX_DATA);

        //Update client data
        data[0] = 0;
        data[1] = 0;
        data[2] = -1;

        break;
      }
    }
	}
  /*===========================================================
  None of the above
  =============================================================*/
	else {
		printf("Error, incorrect command given: %s\n",comm);
	}
	
	//Sends data back to client 
  send(fd, (char*)data, 3*sizeof(int), 0);
  send(fd, list_room, strlen (list_room)+1, 0);

	close(fd);
}

/*-----------------------------------------------------------------------------------
errexit
This just exits the program with an error message for the more serious errors. 
This code was found and copied from the link below:
http://liuj.fcu.edu.tw/tcp_ip_v3/examples/errexit.c.html
---------------------------------------------*/
int errexit(const char *format, ...){
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}
