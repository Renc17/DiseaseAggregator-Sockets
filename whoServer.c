#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define MAX_SIZE 80

int queryPortNum = 0;
int numThreads = 0;
int statisticsPortNum = 0;

void *Connection_handler(void* fds);

//./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize
int main(int argc, char** argv){

    int bufferSize = 0;

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


    int querySocket, statsSocket, workerSocket; //query for whoClient, stats for worker statistics, worker for worker to get query
    int workerQueryPort = 7000;
    struct sockaddr_in server, worker, whoClient;
    int fds[bufferSize];
    memset(fds, -1, sizeof(fds));
    //fds[bufferSize/2] = -1;
    int stats = 0;

    char message[MAX_SIZE];
    int len;

//    pthread_t *thr;
//    if ((thr = malloc(numThreads * sizeof(pthread_t))) == NULL) {
//        perror("malloc");
//        exit(1);
//    }
//
//    for (i = 0; i < numThreads; ++i) {
//        int err;
//        if ((err = pthread_create(&thr[i], NULL, Connection_handler, (void *) fds))) {
//            printf("Invalid : pthread_create %d\n", err);
//            exit(1);
//        }
//    }

    char delim[2];
    memset(delim, '\0', sizeof(delim));
    strcpy(delim, "&");

    while (1) {
        if(stats == 0) {
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
            int clilen = sizeof(worker);
            int newsocket = accept(statsSocket, (struct sockaddr *) &worker, &clilen);

            if (newsocket < 0) {
                perror("can't bind local address");
            }

            i = 0;
            do {
                bzero(message, sizeof(message));
                len = read(newsocket, message, MAX_SIZE);
                if (len != 0) {
                    message[len] = '\0';
                    if(i == 0){
                        workerQueryPort = atoi(message);
                        printf("Server : WorkerPort %d\n", workerQueryPort);
                        i++;
                    } else {
                        printf("%s", message);
                    }
                }
            } while (strcmp(message, "done") != 0);

            printf("\nworkerQueryPort : %d\n", workerQueryPort);
            close(newsocket);
            close(statsSocket);
            stats = 1;
        } else {
            /* open a TCP socket for queries (an Internet stream socket) */
            if((querySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("can't open stream socket");
                exit(1);
            }

            /* bind the local address, so that the client can send to server */
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr("127.0.0.1");
            server.sin_port = htons(queryPortNum);
            printf("Server : Port to whoClient : %d Address %d\n", queryPortNum, server.sin_addr.s_addr);

            if((bind(querySocket, (struct sockaddr *) &server, sizeof(server))) < 0) {
                perror("can't bind local address for querySocket");
                exit(1);
            }

            /* listen to the socket */
            listen(querySocket, numThreads);

            printf("Server : Waiting for whoClient on port %d\n", queryPortNum);
            int clilen = sizeof(whoClient);
            int whoClientSocket = accept(querySocket, (struct sockaddr *) &whoClient, &clilen);
            if (whoClientSocket < 0) {
                perror("can't bind local address");
            }

            //Setup Server
            worker.sin_family = AF_INET;
            worker.sin_addr.s_addr = inet_addr("127.0.0.1");
            worker.sin_port = workerQueryPort;
            printf("Server : Port to worker : %d Address %d\n", workerQueryPort, worker.sin_addr.s_addr);

            if((workerSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                perror("can't open stream socket");
                exit(1);
            }

            printf("Server Connecting to Worker Port : %d\n", worker.sin_port);
            /* connect to the worker */
            if((connect(workerSocket, (struct sockaddr *) &worker, sizeof(worker))) < 0) {
                printf("Server : can't connect to worker (Query), errno %d\n", errno);
                //perror("can't connect to worker (Query)");
                exit(1);
            }

            printf("Server Communicating with worker and whoClient\n");
            do {
                bzero(message, sizeof(message));
                len = read(whoClientSocket, message, MAX_SIZE);       //read Query from whoClient
                /* make sure it's a proper string */
                if (len != 0) {
                    message[len] = '\0';
                    printf("Query : %s\n", message);

                    //Send Query to worker
                    write(workerSocket, message, sizeof(message));
                    bzero(message, sizeof(message));

                    int bytesIn = read(workerSocket, message, sizeof(message));       //read answer from worker
                    write(whoClientSocket, message, sizeof(message));

                    message[bytesIn] = '\0';
                    printf("Server got answer : %s\n", message);

                    close(whoClientSocket);
                    close(workerSocket);
                    return 0;
                }
            } while (1);

            //close(whoClientSocket);
            close(workerSocket);
        }
    }
}

void *Connection_handler(void* fds){
    char message[MAX_SIZE];
    int len;

    int querySocket, statsSocket;
    struct sockaddr_in client, server;
    int *sockets = (int*) fds;

    printf("thread : %ld\n", pthread_self());
    pthread_exit(NULL);
}