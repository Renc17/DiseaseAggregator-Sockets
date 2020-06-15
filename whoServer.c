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

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;         /* Mutex for synchronization */
pthread_mutex_t printMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t avail = PTHREAD_COND_INITIALIZER;
int available = 0;

typedef struct fd{
    int fd;
    int type;
}socketFds;

int numThreads = 0;
int positionFd = 0;
int bufferSize = 0;
int querySocket, statsSocket;         //querySocket for whoClient, statsSocket for worker statistics, workerSocket for worker to get query

void Connection_handler(socketFds* fds, int pos);
void *thread_work(void* args);

//./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize
int main(int argc, char** argv){

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


    struct sockaddr_in server, worker, whoClient;
    socketFds fds[bufferSize];                          //[ fd/type , fd/type , fd/type , ... , fd/type ]
    memset(&fds->type, -1, sizeof(fds));
    memset(&fds->fd, -1, sizeof(fds));


    pthread_t *thread_pool;
    if ((thread_pool = malloc(numThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    int err;
    //printf("Main Thread %ld: Locked the mutex for statistics\n", pthread_self());

    //########### ACCEPT WORKERS FOR STATISTICS ###############//
    for (i = 0; i < numThreads; ++i) {
        if ((err = pthread_create(&thread_pool[i], NULL, thread_work, (void *) fds))) {
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
    //printf("Server : My address %d\n", server.sin_addr.s_addr);

    //bind to statSocket the server address
    if(bind(statsSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address for statsSocket");
        exit(1);
    }

    listen(statsSocket, numThreads);

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

    int numWorkers = 5;
    int stats = 0;

    int pos = 0;
    while (1) {
        if (pos == bufferSize) {
            pos = 0;
        }
        if(stats == 0) {
            for (int j = 0; j < numWorkers; ++j) {
                int clilen = sizeof(worker);
                if (fds[pos].fd == -1) {
                    printf("Server : Waiting for worker to connect(Statistics)\n");
                    fds[pos].fd = accept(statsSocket, (struct sockaddr *) &worker, &clilen);
                    //printf("Server : fd[%d] = %d\n", pos, fds[pos].fd);
                    fds[pos].type = WORKER;
                    if (fds[pos].fd < 0) {
                        perror("can't bind local address");
                        continue;
                    }

                    if ((err = pthread_mutex_lock(&mtx))) {
                        exit(1);
                    }
                    pos++;
                    available++;
                    printf("Server : Accepted a worker - Available socket %d\n", available);
                    pthread_cond_signal(&avail);
                    if ((err = pthread_mutex_unlock(&mtx))) {
                        exit(1);
                    }
                }
            }
            stats = 1;
        } else {
            //############# ACCEPT CLIENT FOR QUERIES ################//

            printf("\nServer : Waiting for whoClient on port %d\n", queryPortNum);

            int clilen = sizeof(whoClient);
            fds[pos].fd = accept(querySocket, (struct sockaddr *) &whoClient, &clilen);
            fds[pos].type = CLIENT;
            if (fds[pos].fd < 0) {
                perror("can't bind local address (whoClient)");
                continue;
            }
            printf("Server : Client accepted : %d, type : %d position : %d\n", fds[pos].fd, fds[pos].type, pos);
            if ((err = pthread_mutex_lock(&mtx))) {
                exit(1);
            }
            pos++;
            available++;

            //printf("Server : Accepted a worker - Available socket %d\n", available);

            pthread_cond_signal(&avail);
            if ((err = pthread_mutex_unlock(&mtx))) {
                exit(1);
            }
        }
    }

    for (i=0 ; i< numThreads ; i++)
        if ((err = pthread_join(*(thread_pool+i), NULL))) {
            exit(1);
        }
    printf("all %d threads have terminated\n", numThreads);

    if ((err = pthread_mutex_destroy(&mtx))) {
        exit(1); }

    if ((err = pthread_mutex_destroy(&printMtx))) {
        exit(1); }

}

void *thread_work(void* args){
    socketFds *sockets = (socketFds *) args;
    int pos;

    while (1) {
        int err;

        if ((err = pthread_mutex_lock(&mtx))) {
            exit(1);
        }

        //printf("Thread %ld : Waiting for available worker. Available %d\n", pthread_self(), available);
        if(available == 0) {
            pthread_cond_wait(&avail, &mtx);
        }

        if( positionFd == bufferSize){
            positionFd = 0;
        }

        pos = positionFd;

        //printf("Thread %ld: Locked the mutex -> position %d\n", pthread_self(), pos);
        //printf("Thread %ld: Read fds %d\n", pthread_self(), sockets[pos].fd);

        positionFd++;
        available--;
        //printf("Thread %ld: position %d available %d\n", pthread_self(), positionFd, available);

        //printf("Thread %ld: Unlocked the mutex\n", pthread_self());
        if ((err = pthread_mutex_unlock(&mtx))) {
            exit(1);
        }

        Connection_handler(sockets, pos);
    }
    pthread_exit(NULL);
}


void Connection_handler(socketFds* fds, int pos){
    char message[MAX_SIZE];
    int len, err;
    int workerQueryPort[numThreads];
    int workerSocket;         //querySocket for whoClient, statsSocket for worker statistics, workerSocket for worker to get query
    struct sockaddr_in worker;

    if (fds[pos].type == WORKER) {
        printf("Thread %ld: Handling connection\n", pthread_self());

        fd_set master, branch;
        FD_ZERO(&master);
        FD_SET(querySocket, &master);
        FD_SET(fds[pos].fd, &master);

        int i = 0;
        do {
            branch = master;
            if((select(FD_SETSIZE, &branch, NULL, NULL, NULL)) < 0){ return; }
            for (int j = 0; j < FD_SETSIZE; ++j) {
                if(FD_ISSET(j, &branch)){
                    if(j == querySocket){
                        printf("Thread is not expecting new connections at this point\n");
                    } else{
                        bzero(message, sizeof(message));
                        len = read(j, message, MAX_SIZE);
                        if (len > 0) {
                            message[len] = '\0';
                            if (i == 0) {
                                if(strcmp(message, "end") == 0){
                                    printf("Thread %ld has no work to do\n", pthread_self());
                                    return;
                                }
                                workerQueryPort[pos] = atoi(message);
                                //printf("Thread %ld WorkerPort %d. Position %d\n", pthread_self(), workerQueryPort[pos], pos);
                                i++;
                            } else {

                                if ((err = pthread_mutex_lock(&printMtx))) {
                                    exit(1);
                                }

                                if (strcmp(message, "done") == 0){
                                    printf("############# HEYYYY I'M %s\n", message);
                                } else{
                                    printf("%s", message);
                                }

                                if ((err = pthread_mutex_unlock(&printMtx))) {
                                    exit(1);
                                }
                            }
                        }else{
                            close(j);
                            FD_CLR(j, &master);
                        }
                    }
                }
            }
        } while (strcmp(message, "done") != 0);

        printf("Thread %ld is clearing fds (Statistics)\n", pthread_self());
        //close(fds[pos].fd);
        fds[pos].fd = -1;
        fds[pos].type = -1;
    }else if( fds[pos].type == CLIENT ) {

        //############# SERVER CONNECT TO WORKER FOR QUERIES #################//
        //Setup Server
        worker.sin_family = AF_INET;
        worker.sin_addr.s_addr = inet_addr("127.0.0.1");
        worker.sin_port = workerQueryPort[pos];
        printf("Server : Port to worker : %d Address %d\n", workerQueryPort[pos], worker.sin_addr.s_addr);

        if ((workerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("can't open stream socket");
            exit(1);
        }

        printf("Server Connecting to Worker Port : %d\n", workerQueryPort[pos]);
        // connect to the worker
        if ((connect(workerSocket, (struct sockaddr *) &worker, sizeof(worker))) < 0) {
            perror("can't connect to worker (Query)");
            exit(1);
        }

        fd_set master, branch;
        FD_ZERO(&master);
        FD_SET(workerSocket, &master);
        FD_SET(fds[pos].fd, &master);

        printf("Server : Thread %ld Communicating with whoClient fd %d position %d\n", pthread_self(), fds[pos].fd,pos);
        do {
            branch = master;
            if((select(FD_SETSIZE, &branch, NULL, NULL, NULL)) < 0){ return; }
            for (int j = 0; j < FD_SETSIZE; ++j) {
                if (FD_ISSET(j, &branch)) {
                    if (j == workerSocket) {
                        //Send Query to worker
                        //write(j, message, sizeof(message));
                        //bzero(message, sizeof(message));
                    } else {
                        bzero(message, sizeof(message));
                        len = read(j, message, MAX_SIZE);       //read Query from whoClient
                        if (len > 0) {
                            message[len] = '\0';
                            printf("Query : %s\n", message);

                            //Send Query to worker
                            write(workerSocket, message, sizeof(message));
                            bzero(message, sizeof(message));

                            int bytesIn = read(workerSocket, message, sizeof(message));       //read answer from worker
                            printf("Server : Thread %ld writing back to his client fd %d pos %d\n", pthread_self(), j, pos);
                            write(j, message, sizeof(message));                               //write answer to client

                            message[bytesIn] = '\0';
                            printf("Server got answer : %s\n", message);

                            bzero(message, sizeof(message));
                            bytesIn = read(j, message, sizeof(message));                     //read terminating message from client
                            message[bytesIn] = '\0';
                            printf("Server got answer : %s\n", message);
                        } else {
                            close(j);
                            FD_CLR(j, &master);
                        }
                    }
                }
            }
        } while (strcmp(message, "done") == 0);
        printf("Thread %ld is clearing fds (Query)\n", pthread_self());
        fds[pos].fd = -1;
        fds[pos].type = -1;
    }
}