#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#define LISTEN_BACKLOG 50
/*
* Queue
*/

/*
* Handlers
*/
static handle_error(const char *msg)
{
	perror(msg);
	pthread_exit(NULL);
}

static int bind_socket(in_port_t port)
{
	int sfd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (sfd == -1) handle_error("socket failure");
	struct sockaddr_in6 ssa;
	memset(&ssa, 0, sizeof(struct sockaddr_in6));
    ssa.sin6_family = AF_INET6;
    ssa.sin6_addr = in6addr_any;
    ssa.sin6_port = port;
    if (setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)NULL, 0) == -1) handle_error("setsockopt failure");
    if (bind(sfd, (struct sockaddr*)&ssa, sizeof(ssa)) == -1) handle_error("bind failure");
    return sfd;
}

static void* accept_connection(void* p)
{
	in_port_t port = *((in_port_t*)p);
	int sfd = bind_socket(port);
    listen(sfd, LISTEN_BACKLOG);
    while(1) {
        struct sockaddr_in6 csa;
        socklen_t csalen = sizeof(csa);
        printf("Handler: accepting connection on port: %hu\n", ntohs(port));
        int cfd = accept(sfd, (struct sockaddr*)&csa, &csalen);
        if (cfd == -1) handle_error("accept failure");
        // receive thread
        // reply tread
        printf("Handler: accepted connection on port: %hu\n", ntohs(port));
    }
	return NULL;
}

/*
* Main
*/
int main(int argc, char* argv[])
{
	if (argc == 1) 
	{
		printf("no port is given.\n");
	}
	else
	{
		int i, rc;
		pthread_t handler_tid[argc - 1];
		in_port_t port[argc - 1];
	   	pthread_attr_t attr;
	    pthread_attr_init(&attr);
 	    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		for (i = 0; i < argc - 1; i++) 
		{
	    	port[i] = htons(atoi(argv[i + 1]));
	    	rc = pthread_create(&handler_tid[i], &attr, accept_connection, (void*)&port[i]);
	    	if (rc == -1) 	
	    	{
	    		printf("failed to start accepting connection pthread_create() is %d\n", rc);
         		exit(-1);
	    	}
	    }
	    pthread_attr_destroy(&attr);
	    for (i = 0; i < argc - 1; i++) 
	    {
	    	rc = pthread_join(handler_tid[i], NULL);
	    	if (rc == -1) 
	    	{
	    		printf("failed to join handler pthread_join() is %d\n", rc);
         		exit(-1);
	    	}
	    	 printf("Server: completed join %d handler on port %hu.", i, ntohs(port[i]));
	    }
	    printf("Server: program completed. Exiting.\n");
	    pthread_exit(NULL);
	}
    return 0;
}