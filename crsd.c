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

int main(int argc, char** argv) {
	char * service; /* service name or port number */
  	int    m_sock, s_sock;      /* master and slave socket     */
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

    	pthread_create(&th, &ta, handle_request, (void*)&s_sock);
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

void *handle_request(void * vfd){
	int fd = *(int*)vfd;
	printf("In Handle Request\n");
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
