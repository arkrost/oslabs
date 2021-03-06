#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <poll.h>
#define LISTEN_BACKLOG 50
#define QUEUE_BACKLOG 50
#define MESSAGE_LIMIT 21
#define INFTIM (-1)
/*
* Queue
*/
typedef struct qpos
{
	int *cfd;
    size_t pos;
    pthread_t receive_tid;
    pthread_t send_tid;
    struct qpos *next;
} qpos_t;

typedef struct message
{
	size_t expected;
	size_t num;
	size_t len;
	pthread_mutex_t mutex;
	char* msg;
} message_t;

static message_t messages[QUEUE_BACKLOG];
static size_t mpos, listener_count;
pthread_rwlock_t global_lock;

static qpos_t *qpos_list;

static qpos_t *new_queue_listener(int *cfd)
{
	pthread_rwlock_wrlock(&global_lock);
	qpos_t *qp = (qpos_t *)malloc(sizeof(qpos_t));
	if (qp)
	{
		qp->pos = mpos;
		qp->next = qpos_list;
		qp->cfd = cfd;
		qpos_list = qp;
		listener_count++;
	}
	pthread_rwlock_unlock(&global_lock);
	return qp;
}

static void free_listeners(qpos_t *listener)
{
	if (listener) 
	{
		free_listeners(listener->next);
		close(*listener->cfd);
		free(listener->cfd);
		free(listener);
	}
}

static size_t next_pos(size_t pos)
{
	return (pos + 1) % QUEUE_BACKLOG;
}

static void new_message(const char *msg, size_t len)
{
	pthread_rwlock_wrlock(&global_lock);
	message_t* msg_container = messages + mpos;
	pthread_mutex_lock(&msg_container->mutex);
	if (msg_container->expected != msg_container->num)
	{
		qpos_t* cur;
		for(cur = qpos_list; cur != NULL; cur = cur->next)
		{
			if (cur->pos == mpos)
			{
				cur->pos = next_pos(mpos);
			}
		}
	}
	msg_container->expected = listener_count;
	msg_container->num = 0;
	memcpy(msg_container->msg, msg, len);
	msg_container->len = len;
	mpos = next_pos(mpos);
	pthread_mutex_unlock(&msg_container->mutex);
	pthread_rwlock_unlock(&global_lock);
}

/*
* Handlers
*/
static void handle_error(const char *msg)
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
    if (setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, NULL, sizeof(NULL)) == -1) handle_error("setsockopt failure");
    if (bind(sfd, (struct sockaddr*)&ssa, sizeof(ssa)) == -1) handle_error("bind failure");
    return sfd;
}

static void* handle_receive(void* qp)
{
	qpos_t* pointer = (qpos_t*)qp;
	if (pointer)
	{
		char buf[MESSAGE_LIMIT];
	    int n;
	    size_t pos, used = 0;
	    char skip = 0;
		struct pollfd poller;
	    poller.fd = *pointer->cfd;
	    poller.events = POLLRDNORM;
	    int loop = 1;
	    while(loop) 
	    { 
	        poll(&poller, 1, INFTIM);
	        if (poller.revents & POLLRDNORM)
	        {
	        	n = read(poller.fd, buf + used, MESSAGE_LIMIT - used);
	        	if (n <= 0) break;
	        	used += n;
	        	pos = 0;
	        	while (pos < used) 
	        	{
		        	for (; pos < used && buf[pos] != '\n'; pos++);
		        	if (pos < used) 
		        	{
			            if (skip) 
			            {
			                skip = 0;
			            } else 
			            {
			            	new_message(buf, pos + 1);
			            }
			            used -= (pos + 1);
			            memmove(buf, buf + pos + 1, used);
			        } 
			        else if (pos == MESSAGE_LIMIT) 
			        {
			            skip = 1;
			            used = 0;
			        }
			    }
	        }
	        pthread_rwlock_rdlock(&global_lock);
	        if (pointer->pos == QUEUE_BACKLOG) loop = 0;
	        pthread_rwlock_unlock(&global_lock);
	    }
	    printf("HR: someone disconnected\n");
	    pthread_rwlock_wrlock(&global_lock);
	    pointer->pos = QUEUE_BACKLOG;
	    pthread_rwlock_unlock(&global_lock);
	}
	return NULL;
}

static void* handle_send(void* qp)
{
	qpos_t* pointer = (qpos_t*)qp;
	if (pointer)
	{
		char buf[MESSAGE_LIMIT];
		size_t len = 0;
		int written;
	    struct pollfd poller;
	    poller.fd = *(pointer->cfd);
	    poller.events = POLLWRNORM;
	    int finish = 0;
	    while (1) {
	    	pthread_rwlock_rdlock(&global_lock);
	    	if (pointer->pos == QUEUE_BACKLOG) 
	    	{
	    		finish = 1;
	    	}
	        else if (pointer->pos != mpos) 
	        {
	            message_t* msg_container = messages + pointer->pos;
	            memcpy(buf, msg_container->msg, msg_container->len);
	            len = msg_container->len;
	            pointer->pos = next_pos(pointer->pos);
	            pthread_mutex_lock(&msg_container->mutex);
	            msg_container->num++;
	            pthread_mutex_unlock(&msg_container->mutex);
	        }
	        pthread_rwlock_unlock(&global_lock);
	        if (finish) break;
	        poll(&poller, 1, INFTIM);
	        if (poller.revents & POLLWRNORM)
	        {
	        	while (len > 0)
		        {
		        	written = write(*(pointer->cfd), buf, len);
		        	if (written <= 0)
		        	{
		        		break;
		        	}
		        	else
		        	{
		        		len -= written;
		        		memmove(buf, buf + written, len);
		        	}
		        }
		        if (len > 0)
		        {
		        	break;
		        }	
	        }
	        len = 0;
	    }
	    printf("HS: someone disconnected\n");
	    pthread_rwlock_wrlock(&global_lock);
	    pointer->pos = QUEUE_BACKLOG;
	    pthread_rwlock_unlock(&global_lock);
	}
	return NULL;
}

static void* accept_connection(void* p)
{
	in_port_t port = *((in_port_t*)p);
	int sfd = bind_socket(port);
    listen(sfd, LISTEN_BACKLOG);
    while(1) {
        struct sockaddr_in6 csa;
        socklen_t csalen = sizeof(csa);
        int* cfd = malloc(sizeof(int));
        *cfd = accept(sfd, (struct sockaddr*)&csa, &csalen);
        if (*cfd == -1) handle_error("accept failure");
        qpos_t* qp = new_queue_listener(cfd);
        int rc;
        rc = pthread_create(&(qp->send_tid), NULL, handle_send, (void*)qp);
    	if (rc == -1) 	
    	{
    		printf("Handler: failed to create sender: pthread_create() is", rc);
     		exit(-1);
    	}
    	rc = pthread_create(&(qp->receive_tid), NULL, handle_receive, (void*)qp);
    	if (rc == -1) 	
    	{
    		printf("Handler: failed to start receiver: pthread_create() is %d\n", rc);
     		exit(-1);
    	}
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
		printf("Server: initilizing...\n");
		//initialization
		mpos = 0;
		qpos_list = NULL;
		listener_count = 0;
		pthread_rwlock_init(&global_lock, NULL);
		size_t qi;
		for (qi = 0; qi < QUEUE_BACKLOG; qi++)
		{
			messages[qi].msg = (char*)malloc(MESSAGE_LIMIT * sizeof(char));
			pthread_mutex_init(&messages[qi].mutex, NULL);
		}
		//start listening
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
	    	printf("Server: start listening port: %hu\n", ntohs(port[i]));
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
	    printf("Server: program completed. Free resources.\n");
	    // free resources
	    for (qi = 0; qi < QUEUE_BACKLOG; qi++)
		{
			free(messages[qi].msg);
			pthread_mutex_destroy(&messages[qi].mutex);
		}
	    free_listeners(qpos_list);
	    pthread_rwlock_destroy(&global_lock);
	    pthread_exit(NULL);
	}
    return 0;
}