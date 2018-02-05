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
void *chatRoom(void * inputData);
void *chat(void * inputData);

/*///////////////////////////////////////////////////////////////////////////////////
Data Structures 
*////////////////////////////////////////////////////////////////////////////////////
struct ServerData{
   int fd; 
   int index; 
   int sock;
   char names[20][MAX_DATA];
   int port[20];
   int members[20];
   bool alive[20];
   int fids[20][20];
   int pids[20];
   int overCount;
};

/*///////////////////////////////////////////////////////////////////////////////////
Main
*////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
  char * service = argv[1]; /* service name or port number */
  int    m_sock, s_sock;      /* master and slave socket     */

  //Setting up the server data struct that holds all information on rooms
  ServerData data;
  data.index = -1;
  data.overCount = 0;
  for(int i = 0; i < 20; i++){
    memset(data.names[i],'\0',MAX_DATA);
    data.port[i] = atoi(argv[1]) + i + 1; //-1 as an invalid address placeholder
    data.pids[i] = -1;
    data.alive[i] = false;
    for(int j = 0; j < 20; j++){
      data.fids[i][j] = -1;
    }
  }
   
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
  int data[3] = {0, 0, 0};
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
        servData->members[i]++;

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
        if(!servData->alive[i] && i == servData->overCount){
          servData->overCount++;
          strcpy(servData->names[i],text[1]);
          printf("Creating room %s on port %d\n", text[1], servData->port[i]);
          servData->index = i;
          //printf("Index set to %d\n", servData->index);
          //convert port number from in to char *
          char roomPort[10];
          memset(roomPort,'\0',10);
          sprintf(roomPort, "%d", servData->port[i]);
          
          int m_sock = passiveTCPsock(roomPort, 32);
          pthread_t th; pthread_attr_t ta;
          pthread_attr_init(&ta);
          pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
          pthread_create(&th, &ta, chatRoom, inputData);

          
          servData->sock = m_sock;
          //printf("sock set to %d\n", servData->sock);
          servData->members[i] = 0;
          servData->alive[i] = true;
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
        servData->alive[i] = false;
        servData->members[i] = 0;
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
  int send_result = send(fd, (char*)data, 3*sizeof(int), MSG_NOSIGNAL);
  if (send_result < 0) { 
    close(fd);
  }
  send_result = send(fd, list_room, strlen (list_room)+1, MSG_NOSIGNAL);
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

/*-----------------------------------------------------------------------------------
chatRoom
This function handles the chat rooms. each room runs this function on its own thread
with its own port. 
---------------------------------------------*/
void *chatRoom(void * inputData){
  //exctract the index for all of the data
  ServerData* servData = (ServerData*)inputData;
  int roomIndex = servData->index;
  int m_sock = servData->sock;
  int s_sock;

  
  while (servData->alive[roomIndex]) {
    
    s_sock = accept(m_sock,NULL,NULL);
    if (s_sock < 0){
      if (errno == EINTR) continue;
      else errexit("accept failed: %s\n", strerror(errno));
    }
    
    fflush(stdout);
    for(int i = 0; i < 20; i++){
      if(servData->fids[roomIndex][i] == -1){
        servData->fids[roomIndex][i] = s_sock;
        servData->fd = s_sock;
        break;
      }
    }
    pthread_t th; pthread_attr_t ta;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    pthread_create(&th, &ta, chat, inputData);
  }
  pthread_exit(NULL);
}


/*-----------------------------------------------------------------------------------
chat
This function handles the chat input and output taken from the clients. Each client
gets their own thread with this running. 
---------------------------------------------*/
void *chat(void * inputData){
  //exctract the index for all of the data
  ServerData* servData = (ServerData*)inputData;
  int roomIndex = servData->index;
  int fd = servData->fd;
  char comm[MAX_DATA];
  memset(comm,'\0',MAX_DATA);
  bool death = false; 
  
  //printf("in chat\n");
  while (servData->alive[roomIndex] && !death) {
    // Get message
    memset(comm,'\0',MAX_DATA);
    if(recv(fd, comm, MAX_DATA,0) < 0){
      death = true;
    }
    for(int i = 0; i < 20; i++){

      if(servData->fids[roomIndex][i] != fd && servData->fids[roomIndex][i] != -1){
      
        int send_result = send(servData->fids[roomIndex][i], comm, strlen (comm)+1, MSG_NOSIGNAL);

        if (send_result < 0) { 
          death = true;
          servData->members[roomIndex]--;
          if(servData->fids[roomIndex][i] == fd){
            printf("Tis marks the death of %d...", fd);
            servData->fids[roomIndex][i] = -1;
          }
          close(fd);
        }
      }
    }
    
  }
}