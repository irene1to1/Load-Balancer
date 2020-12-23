#include<err.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<getopt.h>
#include<pthread.h>
#include<stdbool.h>

#define BUFFER_SIZE 4096

pthread_mutex_t h_lock       = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t h_condition   = PTHREAD_COND_INITIALIZER;

struct Node{
	int data1;
	int data2;
	struct Node* next;
};

struct Queue{
	struct Node *front, *rear;

	pthread_mutex_t lock;
	pthread_cond_t condition;
};

struct Node* newNode(int k, int l){
	struct Node* temp = (struct Node*)malloc(sizeof(struct Node));
    temp->data1 = k;
	temp->data2 = l;
    temp->next = NULL;
    return temp;
}

struct Queue* createQueue()
{
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
	
	pthread_mutex_init(&(q->lock), NULL);
	pthread_cond_init(&(q->condition), NULL);

    return q;
}
  
// The function to add a data k to q
void enQueue(struct Queue* q, int k, int l)
{
    // Create a new LL node
    struct Node* temp = newNode(k, l);
  
    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        return;
    }
    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
}

void deQueue(struct Queue* q)
{
    // If queue is empty, return NULL.
    if (q->front == NULL)
        return;
  

    // Store previous front and move front one node ahead
    struct Node* temp = q->front;
  
    q->front = q->front->next;
  
    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
  
    free(temp);
}

int getCount(struct Node* front)
{
    int count = 0;  // Initialize count
    struct Node* current = front;
	//printf("current->data: %d\n", current->data); 
    while (current != NULL)
    {
        count++;
		//printf("just incremented %d\n", count);
        current = current->next;
    }
    return count;
}

typedef struct server_info_t{
	int port;
	bool alive;
	int total_requests;
	int error_requests;
	
} ServerInfo;

ServerInfo *servers;

/*
typedef struct servers_t{
	//int numServers;
	ServerInfo *server;
	pthread_mutex_t mut;
} Servers;

Servers *servers;
*/
int numServers;

int init_servers(int argc, char **argv, int start){
	//printf("this is argc: %d, this is start %d\n", argc, start);
	numServers = argc - start;
	if(numServers <= 0){
		return -1;
	}
	servers = malloc(numServers * sizeof(ServerInfo));
	//pthread_mutex_init(&servers->mut, NULL);
	for(int i = start; i < argc; i++){
		int j = i - start;
		servers[j].port = atoi(argv[i]);
		servers[j].alive = false;
		servers[j].error_requests = 0;
		servers[j].total_requests = 0;
		printf("connectport[%d] number : %d\n", j, servers[j].port);
	}
	return 0;
}
/*
 * client_connect takes a port number and establishes a connection as a client.
 * connectport: port number of server to connect to
 * returns: valid socket if successful, -1 otherwise
 */
int client_connect(uint16_t connectport) {
    int connfd;
    struct sockaddr_in servaddr;

    connfd=socket(AF_INET,SOCK_STREAM,0);
    if (connfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);

    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(connectport);

    /* For this assignment the IP address can be fixed */
    inet_pton(AF_INET,"127.0.0.1",&(servaddr.sin_addr));

    if(connect(connfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0)
        return -1;
    return connfd;
}

/*
 * server_listen takes a port number and creates a socket to listen on 
 * that port.
 * port: the port number to receive connections
 * returns: valid socket if successful, -1 otherwise
 */
int server_listen(int port) {
    int listenfd;
    int enable = 1;
    struct sockaddr_in servaddr;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
        return -1;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        return -1;
    if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof servaddr) < 0)
        return -1;
    if (listen(listenfd, 500) < 0)
        return -1;
    return listenfd;
}

/*
 * bridge_connections send up to 100 bytes from fromfd to tofd
 * fromfd, tofd: valid sockets
 * returns: number of bytes sent, 0 if connection closed, -1 on error
 */
int bridge_connections(int fromfd, int tofd) {
    char recvline[100];
    int n = recv(fromfd, recvline, 100, 0);
    if (n < 0) {
        //printf("connection error receiving\n");
        return -1;
    } else if (n == 0) {
        //printf("receiving connection ended\n");
        return 0;
    }
    recvline[n] = '\0';
    //printf("%s", recvline);
    //sleep(1);
    n = send(tofd, recvline, n, 0);
    if (n < 0) {
        //printf("connection error sending\n");
        return -1;
    } else if (n == 0) {
        //printf("sending connection ended\n");
        return 0;
    }
    return n;
}

/*
 * bridge_loop forwards all messages between both sockets until the connection
 * is interrupted. It also prints a message if both channels are idle.
 * sockfd1, sockfd2: valid sockets
 */
void bridge_loop(int sockfd1, int sockfd2) {
    fd_set set;
    struct timeval timeout;

    int fromfd, tofd;
    while(1) {
        // set for select usage must be initialized before each select call
        // set manages which file descriptors are being watched
        FD_ZERO (&set);
        FD_SET (sockfd1, &set);
        FD_SET (sockfd2, &set);

        // same for timeout
        // max time waiting, 5 seconds, 0 microseconds
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

		char *responseHeader;
		responseHeader = (char*)"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
        // select return the number of file descriptors ready for reading in set
        switch (select(FD_SETSIZE, &set, NULL, NULL, &timeout)) {
            case -1:
                //printf("error during select, exiting\n");
				send(sockfd1, responseHeader, strlen(responseHeader), 0);
                return;
            case 0:
                //printf("both channels are idle, waiting again\n");
				send(sockfd1, responseHeader, strlen(responseHeader), 0);
                continue;
            default:
                if (FD_ISSET(sockfd1, &set)) {
                    fromfd = sockfd1;
                    tofd = sockfd2;
                } else if (FD_ISSET(sockfd2, &set)) {
                    fromfd = sockfd2;
                    tofd = sockfd1;
                } else {
                    //printf("this should be unreachable\n");
                    return;
                }
        }
        if (bridge_connections(fromfd, tofd) <= 0)
            return;
    }
}
void* handle_health(){
	int client_sockd = -1;

	struct timespec clock;
	clock_gettime (CLOCK_REALTIME, &clock);
	while(true){
		pthread_mutex_lock(&h_lock);
		pthread_cond_timedwait(&h_condition, &h_lock, &clock);

		clock_gettime(CLOCK_REALTIME, &clock);
		clock.tv_sec += 20;
		//int alldown = 0;
		//printf("this is length of alldown %ld\n", strlen(alldown[1]));
		printf("--------------Healthchecking--------------\n");
		for(int i = 0; i < numServers; i++){
			client_sockd = client_connect(servers[i].port);
			if (client_sockd < 0){
				servers[i].alive = false;
				servers[i].total_requests = 0;
				servers[i].error_requests = 0;
				/*alldown += 1;
				if(alldown == (numServers)){
					char *responseHeader;
					responseHeader = (char*)"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
					send(client_sockd, responseHeader, strlen(responseHeader), 0);
				}*/
				printf("*connect port[%d] is not up*\n", servers[i].port);

			}else{
				send(client_sockd, "GET /healthcheck HTTP/1.1\r\nContent-Length: 0\r\n\r\n", 48, 0);

                fd_set set;
                struct timeval timeout;
				// set for select usage must be initialized before each select call
        		// set manages which file descriptors are being watched
                FD_ZERO (&set);
                FD_SET (client_sockd, &set);

                timeout.tv_sec = 5;
                timeout.tv_usec = 0;
                if (select(FD_SETSIZE, &set, NULL, NULL, &timeout) == 0) {
                    servers[i].alive = false;
					servers[i].total_requests = 0;
					servers[i].error_requests = 0;
					//printf("test if it goes in\n");
                    continue;
                }
				
				char response[BUFFER_SIZE];
				char buffed[BUFFER_SIZE];
				response[0] = '\0';
                while(true) {
                    int bytes = recv(client_sockd, &buffed, BUFFER_SIZE, 0);
                    buffed[bytes] = '\0';
                    strcat(response, buffed);
                    if (bytes == 0){
						break;
					}
					//printf("test if it goes in6\n");
                }
                int status;
				int entries = 0;
				int errors = 0;

                sscanf(response, "HTTP/1.1 %d %s\r\nContent-Length: %s\r\n\r\n%d\n%d", &status, buffed, buffed, &errors, &entries);
				printf("this is status %d, this error %d, this entries %d\n", status, errors, entries);
                if (status != 200) {
                    servers[i].alive = false;
					servers[i].total_requests = 0;
					servers[i].error_requests = 0;
                } else {
                    servers[i].alive = true;
					servers[i].total_requests = entries;
					servers[i].error_requests = errors;
                }
			}		
		}
		pthread_mutex_unlock(&h_lock);
	}
}

void* handle_task(void* thread){
	struct Queue* w_thread = (struct Queue*)thread;
	int acceptfd_sockd = -1;
	int connfd_sockd = -1;

	while(true){
		//printf("[+]Worker thread [%li]: ready for a task\n", pthread_self());
		pthread_mutex_lock(&(w_thread->lock));
		while(getCount(w_thread->front) == 0){
			pthread_cond_wait(&(w_thread->condition), &(w_thread->lock));
		}
		//printf("[-]Worker thread [%li]: handling socket: %d\n", pthread_self(), w_thread->front->data1);
		if(w_thread->front != NULL){
			acceptfd_sockd = w_thread->front->data1;
			connfd_sockd = w_thread->front->data2;
			deQueue(w_thread);
		}
		pthread_mutex_unlock(&(w_thread->lock));
		
		bridge_loop(acceptfd_sockd, connfd_sockd);
	}
}

int main(int argc,char **argv) {
    int connfd, listenfd, acceptfd;
    uint16_t listenport;

    if (argc < 3) {
        //printf("missing arguments: usage %s port_to_connect port_to_listen", argv[0]);
        return 1;
    }
	int c;
	int numThread = 4;
	int numRequest = 5;
	int sofar = 0;
	while(1){
		c = getopt(argc, argv, "N:R:");
		if(c == -1){ break; }
		if(c == 'N'){
			numThread = atoi(optarg);
			if(numThread < 1){
				//printf("Error: not enough threads");
				return -1;	
			}
			//printf("N : %d\n", numThread);
		}else if(c == 'R'){
			numRequest = atoi(optarg);
			if(numRequest < 1){
				//printf("Error: X second is not valid");
				return -1;	
			}
			//printf("R : %d\n", numRequest);
		}else{
			//printf("Error: bad arguments");
			return -1;
		}
	}
	//char *endptr;
	//int temp = optind; 
	int ret;
	if(optind != argc){
		listenport = atoi(argv[optind]);
		//printf("listenport number : %d\n", listenport);
		//printf("HERE\n");
		ret = init_servers(argc, argv, optind + 1);
	}

	if ((listenfd = server_listen(listenport)) < 0)
    	return 0;

	struct Queue* q = createQueue();

	int is_error = 0;
	pthread_t workers[numThread];
	for(int i = 0; i < numThread; i++){
		is_error = pthread_create(&workers[i], NULL, handle_task, q);

		if(is_error){
			//printf("error creating some threads\n");
			return -1;
		}
	}
	
	pthread_t health_thread;
	is_error = pthread_create(&health_thread, NULL, handle_health, servers);

	if(is_error){
		//printf("error creating health thread\n");
		return -1;
	}

	while(1){
		if ((acceptfd = accept(listenfd, NULL, NULL)) < 0){
    	    //err(1, "failed accepting");
		}else{
			sofar++;
		}
	

		if(sofar % numRequest == 0){
			pthread_mutex_lock(&h_lock);
			pthread_cond_signal(&h_condition);
			pthread_mutex_unlock(&h_lock);
		}

		int least = -1;
		pthread_mutex_lock(&h_lock);
		while(true){
			for(int i = 0; i < numServers; i++){
				if(servers[i].alive){
					//printf("test if it goes in\n");
					if(least == -1){
						least = i;
						//printf("test if it goes in\n");							
					}else if(servers[i].total_requests < servers[least].total_requests){
						least = i;
						//printf("test if it goes in************************\n");
					}else if(servers[i].total_requests == servers[least].total_requests){
						if(servers[i].error_requests > servers[least].error_requests){
							// tiebreaker: more errors, more bandwidth
							least = i;
						}
					}
				}
			}
			//printf("test port: %d\n", servers[least].port);
			connfd = client_connect(servers[least].port);
			if (connfd < 0){
    	    	//err(1, "failed connecting");
				if(least == -1 || ret == -1){
					char *responseHeader;
					responseHeader = (char*)"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
					send(acceptfd, responseHeader, strlen(responseHeader), 0);
					break;
				}

				servers[least].alive = false;
				//printf("failed connecting\n");
				least = -1;
				continue;
			}else{
				break;
			}
		}
		servers[least].total_requests += 1;
		pthread_mutex_unlock(&h_lock);
		printf("chosen port: %d\n", servers[least].port);	

		pthread_mutex_lock(&(q->lock));
		enQueue(q, acceptfd, connfd);
		pthread_cond_signal(&(q->condition));
		pthread_mutex_unlock(&(q->lock));
		
	}
	return 1;
    // This is a sample on how to bridge connections.
    // Modify as needed.
    //bridge_loop(acceptfd, connfd);
}

