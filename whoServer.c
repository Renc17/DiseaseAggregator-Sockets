#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_SIZE 80

void *GetClientQuery(void* fds);

//./whoServer –q queryPortNum -s statisticsPortNum –w numThreads –b bufferSize
int main(int argc, char** argv){

    int queryPortNum = 0;
    int bufferSize = 0;
    int numThreads = 0;
    int statisticsPortNum = 0;

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


    int querySocket, statsSocket;
    struct sockaddr_in client, server;
    int fds[bufferSize];
    //fds[bufferSize/2] = -1;

    char string[MAX_SIZE];
    int len;

    /*pthread_t *thr;
    if ((thr = malloc(numThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    for (i = 0; i < numThreads; ++i) {
        int err;
        if ((err = pthread_create(&thr[i], NULL, GetClientQuery, (void *) fds))) {
            printf("Invalid : pthread_create %d\n", err);
            exit(1);
        }
    }*/

    /* open a TCP socket for queries (an Internet stream socket) */
//    if((querySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//        perror("can't open stream socket");
//        exit(1);
//    }

    /* open a TCP socket for statistics (an Internet stream socket) */
    if((statsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }
    printf("StatsSocket %d\n", statsSocket);

    /* bind the local address, so that the client can send to server */
//    server.sin_family = AF_INET;
//    server.sin_addr.s_addr = inet_addr("127.0.0.1");
//    server.sin_port = htons(queryPortNum);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr( "127.0.0.1");
    server.sin_port = htons(statisticsPortNum);
    printf("Socket Port (Statistics): %d\n", server.sin_port);

//    if(bind(querySocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
//        perror("can't bind local address for querySocket");
//        exit(1);
//    }

    if(bind(statsSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address for statsSocket");
        exit(1);
    }

    /* listen to the socket */
//    listen(querySocket, numThreads);
    listen(statsSocket, numThreads);

    i = 0;
    while (1) {

        /* wait for a connection from a client; this is an iterative server */
        int clilen = sizeof(client);
        int newsocket = accept(statsSocket, (struct sockaddr *) &client, &clilen);

        if (newsocket < 0) {
            perror("can't bind local address");
        }

        /* read a message from the client */
        do {
            bzero(string, sizeof(string));
            len = read(newsocket, string, MAX_SIZE);
            /* make sure it's a proper string */
            if(len != 0) {
                string[len] = 0;
                printf("%s", string);
            }
        }while (strcmp(string, "done") != 0);
        //if (sendto(fds[i], "done", 5, 0, (struct sockaddr *) &client, sizeof(client))<0)
        //    exit(1);
        close(newsocket);
    }


    return 0;
}

void *GetClientQuery(void* fds){
    char string[MAX_SIZE];
    int len;

    int socket = *(int*) fds;
    printf("thread : %ld\n", pthread_self());
    if(socket != -1){
        /* read a message from the client */
        len = read(socket, string, MAX_SIZE);
        /* make sure it's a proper string */
        string[sizeof(string)-1]='\0';
        printf("%s\n", string);
//    if (sendto(socket, "done", 5, 0, (struct sockaddr *) &client, sizeof(client))<0)
//        exit(1);
    }
    pthread_exit(NULL);
}