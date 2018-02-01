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

int passiveTCPsock(const char * service, int backlog);
void *handle_request(void * fd);
int errexit(const char *format, ...);

struct ServerData{
   int fd; 
   char names[20][MAX_DATA];
   int port[20];
   int members[20];
};

int main(int argc, char** argv) {
	char * service; /* service name or port number */
  int    m_sock, s_sock;      /* master and slave socket     */
  ServerData data;
  for(int i = 0; i < 20; i++){
    memset(data.names[i],'\0',MAX_DATA);
    data.port[i] = -1;
  }
  
  	//struct sockaddr_storage fsin;
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

void *handle_request(void * inputData){
    ServerData* servData = (ServerData*)inputData;
    int fd = servData->fd;

    //send [0] = enum, [1] = # members, [3] = port #
    int data[3] = {0, 15, 3005};

    printf("In Handle Request\n");

    /* dont think this is necessary****
    struct sockaddr_storage client_addr;
    socklen_t addr_size = sizeof client_addr;
    int new_fd = accept(fd, (struct sockaddr *)&client_addr, &addr_size);
    if(new_fd == -1){
        printf("Failed creating new fd\n");
        fflush(stdout);
    }
    **********************************/

    printf("connected successfully\n");
    fflush(stdout);
	
	// Default data
    int status = 0;
    int num = 15;
    int port = 3001;
    char list_room[MAX_DATA];
    memset(list_room,'\0',MAX_DATA);

	// Get command
	char comm[MAX_DATA];
	recv(fd, comm, MAX_DATA,0);
	
	// Seperate room # and command
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

	// Join
  //data[2] = port # if found
  //data[1] = # of members 
  //data[0] = FAILURE_NOT_EXISTS if not
	if (strncmp(comm, "JOIN", 4) == 0) {
		printf("Joining room %s\n", text[1]);

		//Search list of rooms
    bool exists = false; 
    for(int i = 0; i < 20; i++){
      if (strncmp(text[1],servData->names[i],5) == 0){
        exists = true; 

        //Update server data
        servData->members[i] = 1;

        //Update client data
        data[0] = 0;
        data[1] = servData->members[i];
        data[2] = servData->port[i];

        //TODO Make the join happen; 
        break;
      }
    }

    //Room not found case
    if(!exists){
      data[0] = FAILURE_NOT_EXISTS;
    }
	} 
	// List
	else if (strncmp(comm, "LIST", 4) == 0) {
		printf("Listing rooms\n");
		
    int index = 0;
    for(int i = 0; i < 20; i++){
      int j = 0;
      while(isalpha(servData->names[i][j]) || isdigit(servData->names[i][j])){
        list_room[index] = servData->names[i][j];
        index++;
        j++;
      }
      
      if(list_room[index-1] != ','){
        list_room[index] = ',';
        index++;
      }
    }
    printf("\n%s\n\n", list_room);
	}

	// Create
  //data[0] = success/fail
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
	// Delete
	else if (strncmp(comm, "DELETE", 6) == 0) {
		printf("Deleting room %s\n", text[1]);
		
		//TODO:
		//Check if room exists
		//delete room
		//data[0] = success/fail
	}
	else {
		printf("Error, incorrect command given: %s\n",comm);
	}
	
	//Sends requested data
    send(fd, (char*)data, 3*sizeof(int), 0);
    send(fd, list_room, strlen (list_room)+1, 0);

	close(fd);
}

//Found at: http://liuj.fcu.edu.tw/tcp_ip_v3/examples/errexit.c.html
int errexit(const char *format, ...){
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}
