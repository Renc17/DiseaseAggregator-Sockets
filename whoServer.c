#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_SIZE 80
#define CLIENT 1
#define WORKER 0

pthread_mutex_t mtx;         /* Mutex for synchronization */
pthread_mutex_t printMtx;

typedef struct fd{
    int fd;
    int type;
}socketFds;

int numThreads = 0;
int positionFd = 0;


void *Connection_handler(void* fds);

//./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize
int main(int argc, char** argv){

    int bufferSize = 0;
    int statisticsPortNum = 0;
    int queryPortNum = 0;

    int i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-q") != 0) {
        i++;
    }
    queryPortNum = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-b") != 0) {
        i++;
    }
    bufferSize = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-w") != 0) {
        i++;
    }
    numThreads = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-s") != 0) {
        i++;
    }
    statisticsPortNum = atoi(argv[i + 1]);


    int querySocket, statsSocket, workerSocket;         //querySocket for whoClient, statsSocket for worker statistics, workerSocket for worker to get query
    struct sockaddr_in server, worker, whoClient;
    socketFds fds[bufferSize];                          //[ fd/type , fd/type , fd/type , ... , fd/type ]
    memset(fds, -1, sizeof(fds));
    pthread_mutex_init(&mtx, NULL);
    pthread_mutex_init(&printMtx, NULL);


    pthread_t *thr;
    if ((thr = malloc(numThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    int err;
    if ((err = pthread_mutex_lock(&mtx))) {         /* Lock mutex */
        exit(1);
    }
    printf("Main Thread %ld: Locked the mutex\n", pthread_self());

    for (i = 0; i < numThreads; ++i) {
        if ((err = pthread_create(&thr[i], NULL, Connection_handler, (void *) fds))) {
            printf("Invalid : pthread_create %d\n", err);
            exit(1);
        }
    }

    char message[MAX_SIZE];
    int len;

    if((statsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(statisticsPortNum);
    printf("Server : My address %d\n", server.sin_addr.s_addr);

    //bind to statSocket the server address
    if(bind(statsSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address for statsSocket");
        exit(1);
    }

    listen(statsSocket, numThreads);

    printf("Server : Waiting for worker to connect(Statistics)\n");
    for (int j = 0; j < numThreads; ++j) {
        int clilen = sizeof(worker);
        fds[j].fd = accept(statsSocket, (struct sockaddr *) &worker, &clilen);
        printf("Server : fd[%d] = %d\n", j, fds[j].fd);
        fds[j].type = WORKER;

        if (fds[j].fd < 0) {
            perror("can't bind local address");
            exit(1);
        }
    }


    printf("Main Thread %ld: Unlocked the mutex\n", pthread_self());
    if ((err = pthread_mutex_unlock(&mtx))) {           /* Unlock mutex */
        exit(1);
    }

    for (i=0 ; i< numThreads ; i++)
        if ((err = pthread_join(*(thr+i), NULL))) {
            exit(1);
        }
    printf("all %d threads have terminated\n", numThreads);

    if ((err = pthread_mutex_destroy(&mtx))) {
        exit(1); }

    if ((err = pthread_mutex_destroy(&printMtx))) {
        exit(1); }


    /*char delim[2];
    memset(delim, '\0', sizeof(delim));
    strcpy(delim, "&");
    int firstConnection = 0;*/

    /*while (1) {

            // open a TCP socket for queries (an Internet stream socket)
            if((querySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("can't open stream socket");
                exit(1);
            }

            // bind the local address, so that the client can send to server
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr("127.0.0.1");
            server.sin_port = htons(queryPortNum);
            //printf("Server : Port to whoClient : %d Address %d\n", queryPortNum, server.sin_addr.s_addr);

            if((bind(querySocket, (struct sockaddr *) &server, sizeof(server))) < 0) {
                perror("can't bind local address for querySocket");
                exit(1);
            }

            // listen to the socket
            listen(querySocket, numThreads);

            //printf("\nServer : Waiting for whoClient on port %d\n", queryPortNum);
            int clilen = sizeof(whoClient);

            int whoClientSocket = accept(querySocket, (struct sockaddr *) &whoClient, &clilen);
            if (whoClientSocket < 0) {
                perror("can't bind local address");
            }

            if(firstConnection == 0) {
                //Setup Server
                worker.sin_family = AF_INET;
                worker.sin_addr.s_addr = inet_addr("127.0.0.1");
                worker.sin_port = workerQueryPort;
                printf("Server : Port to worker : %d Address %d\n", workerQueryPort, worker.sin_addr.s_addr);

                if ((workerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    perror("can't open stream socket");
                    exit(1);
                }

                //printf("Server Connecting to Worker Port : %d\n", worker.sin_port);
                // connect to the worker
                if ((connect(workerSocket, (struct sockaddr *) &worker, sizeof(worker))) < 0) {
                    printf("Server : can't connect to worker (Query), errno %d\n", errno);
                    //perror("can't connect to worker (Query)");
                    exit(1);
                }
                firstConnection = 1;
            }

            //printf("Server Communicating with worker and whoClient\n");
            do {
                bzero(message, sizeof(message));
                len = read(whoClientSocket, message, MAX_SIZE);       //read Query from whoClient
                // make sure it's a proper string
                if (len != 0) {
                    message[len] = '\0';
                    printf("Query : %s\n", message);

                    //Send Query to worker
                    write(workerSocket, message, sizeof(message));
                    bzero(message, sizeof(message));

                    int bytesIn = read(workerSocket, message, sizeof(message));       //read answer from worker
                    write(whoClientSocket, message, sizeof(message));

                    message[bytesIn] = '\0';
                    //printf("Server got answer : %s\n", message);

                    bzero(message, sizeof(message));
                    bytesIn = read(whoClientSocket, message, sizeof(message));       //read answer from worker
                    message[bytesIn] = '\0';
                    //printf("Server got answer : %s\n", message);
                }
            } while (strcmp(message, "done") == 0);

            close(whoClientSocket);
            close(querySocket);
        //}
    }*/
    //close(workerSocket);
}

void *Connection_handler(void* fds){
    char message[MAX_SIZE];
    int len;

    int querySocket, statsSocket;
    struct sockaddr_in worker, server;
    socketFds *sockets = (socketFds *) fds;
    int pos;
    int workerQueryPort[numThreads];

    printf("thread : %ld\n", pthread_self());

    int err;
    printf("Thread %ld: Trying to lock the mutex\n", pthread_self());

    if ((err = pthread_mutex_lock(&mtx))) {
        exit(1);
    }

    pos = positionFd;

    printf("Thread %ld: Locked the mutex -> position %d\n", pthread_self(), pos);
    printf("Thread %ld: Read fds %d\n", pthread_self(), sockets[pos].fd);

    positionFd++;
    printf("Thread %ld: position %d\n", pthread_self(), positionFd);

    printf("Thread %ld: Unlocked the mutex\n", pthread_self());
    if ((err = pthread_mutex_unlock(&mtx))) {
        exit(1);
    }

    if(sockets[pos].type == WORKER) {
        int i = 0;
        do {
            bzero(message, sizeof(message));
            len = read(sockets[pos].fd, message, MAX_SIZE);
            if (len != 0) {
                message[len] = '\0';
                if (i == 0) {
                    workerQueryPort[pos] = atoi(message);
                    //printf("Server : WorkerPort %d\n", workerQueryPort);
                    i++;
                } else {

                    if ((err = pthread_mutex_lock(&printMtx))) {         /* Lock mutex */
                        exit(1);
                    }

                    printf("%s", message);

                    if ((err = pthread_mutex_unlock(&printMtx))) {           /* Unlock mutex */
                        exit(1);
                    }
                }
            }
        } while (strcmp(message, "done") != 0);

        printf("\nworkerQueryPort : %d\n", workerQueryPort[pos]);
        close(sockets[pos].fd);
    } else{
        printf("CLIENT\n");
    }

    pthread_exit(NULL);
}