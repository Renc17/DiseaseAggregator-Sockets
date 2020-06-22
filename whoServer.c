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
#define MAX_NUM_WORKERS 100

pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t avail = PTHREAD_COND_INITIALIZER;
pthread_mutex_t positionFdMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t availableMutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t workerCountMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t workerSocketMutex = PTHREAD_MUTEX_INITIALIZER;

int available = 0;

typedef struct socketFds{
    int fd;
    int type;
}socketFds;

int workerCount = 0;
int connectedToWorker = 0;
int numThreads = 0;
int bufferSize = 0;

int workerSocket[MAX_NUM_WORKERS];
int workerQueryPort[MAX_NUM_WORKERS];            // max number of workers that server can handle 100

socketFds *fds;
int read_write_thread_fd = 0;

fd_set masterWorker, branchWorker;
struct sockaddr_in worker_addr;

void Connection_handler(socketFds* fd, int pos);
void *thread_work();
void SetUpWorker();

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


    fds = malloc(sizeof(*fds)*bufferSize);
    for (i = 0; i < bufferSize; ++i) {
        fds[i].fd = -1;
        fds[i].type = -1;
    }


    pthread_t *thread_pool;
    if ((thread_pool = malloc(numThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    int err;
    for (i = 0; i < numThreads; ++i) {
        if ((err = pthread_create(&thread_pool[i], NULL, thread_work, NULL))) {
            printf("Invalid : pthread_create %d\n", err);
            exit(1);
        }
    }



    int querySocket, statsSocket;         //querySocket for whoClient, statsSocket for worker statistics, workerSocket for worker to get query

    //########### SET UP SERVER FOR STATISTICS AND QUERY ###############//

    if((statsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    if((querySocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    struct sockaddr_in server, worker, whoClient;

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(statisticsPortNum);
    //printf("Server : My address %d\n", server.sin_addr.s_addr);

    //bind to statSocket the server address
    if(bind(statsSocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address for statsSocket");
        exit(1);
    }

    server.sin_port = htons(queryPortNum);
    if(bind(querySocket, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address for statsSocket");
        exit(1);
    }

    listen(statsSocket, 100);
    listen(querySocket, 100);

    //################ RUN SERVER FOR STATISTICS AND QUERY #################//

    int workerLen = sizeof(worker);
    int clientLen = sizeof(whoClient);
    int positionFd = 0;

    fd_set acceptSelect;
    FD_ZERO(&acceptSelect);
    FD_SET(statsSocket, &acceptSelect);
    FD_SET(querySocket, &acceptSelect);

    while (1){

        fd_set branchAcceptSelect = acceptSelect;
        if((select(FD_SETSIZE, &branchAcceptSelect, NULL, NULL, NULL)) < 0){ exit(1); }
        for (int j = 0; j < FD_SETSIZE; ++j) {
            if(FD_ISSET(j, &branchAcceptSelect)) {
                if(j == statsSocket){
                    if (fds[positionFd].fd == -1) {

                        pthread_mutex_lock(&printMutex);
                        printf("Server : Waiting for worker to connect(Statistics)\n");
                        pthread_mutex_unlock(&printMutex);

                        fds[positionFd].fd = accept(statsSocket, (struct sockaddr *) &worker, &workerLen);
                        worker_addr = worker;

                        fds[positionFd].type = WORKER;
                        if (fds[positionFd].fd < 0) {
                            perror("can't bind local address");
                            continue;
                        }

                        positionFd++;
                        if (positionFd == bufferSize-1) {
                            positionFd = 0;
                        }

                        pthread_mutex_lock(&availableMutex);
                        available++;

                        pthread_cond_signal(&avail);
                        pthread_mutex_unlock(&availableMutex);
                    }
                } else if(j == querySocket){

                    if(connectedToWorker == 0){
                        SetUpWorker();
                        connectedToWorker = 1;
                    }

                    if (fds[positionFd].fd == -1) {

                        pthread_mutex_lock(&printMutex);
                        printf("Server : Waiting for whoClient to connect (Query)\n");
                        pthread_mutex_unlock(&printMutex);

                        fds[positionFd].fd = accept(querySocket, (struct sockaddr *) &whoClient, &clientLen);

                        fds[positionFd].type = CLIENT;
                        if (fds[positionFd].fd < 0) {
                            perror("can't bind local address");
                            continue;
                        }

                        positionFd++;
                        if (positionFd == bufferSize-1) {
                            positionFd = 0;
                        }

                        pthread_mutex_lock(&availableMutex);
                        available++;

                        pthread_cond_signal(&avail);
                        pthread_mutex_unlock(&availableMutex);
                    }
                }
            }
        }
    }


    for (i=0 ; i< numThreads ; i++)
        if ((err = pthread_join(*(thread_pool+i), NULL))) {
            exit(1);
        }
    printf("all %d threads have terminated\n", numThreads);

    if ((err = pthread_mutex_destroy(&availableMutex))) {
        exit(1); }

    if ((err = pthread_mutex_destroy(&positionFdMutex))) {
        exit(1); }

    if ((err = pthread_mutex_destroy(&printMutex))) {
        exit(1); }

    free(fds);

}

void *thread_work(){
    int pos;

    while (1) {
        pthread_mutex_lock(&availableMutex);
        if(available == 0) {
            pthread_cond_wait(&avail, &availableMutex);         //releases availableMutex and lock it back when called
        }

        pthread_mutex_lock(&positionFdMutex);
        pos = read_write_thread_fd;
        read_write_thread_fd++;
        if( read_write_thread_fd == bufferSize-1){
            read_write_thread_fd = 0;
        }
        pthread_mutex_unlock(&positionFdMutex);

        available--;
        //printf("Thread %ld: I got a worker/whoClient %d, Available %d\n", pthread_self(), sockets[pos].fd, available);

        //printf("\n\nThread : %ld unlocked mtx\n", pthread_self());
        pthread_mutex_unlock(&availableMutex);

        //printf("Thread %ld: Calling Connection_handler for worker/whoClient %d\n", pthread_self(), sockets[pos].fd);
        Connection_handler(fds, pos);
    }
    pthread_exit(NULL);
}


void Connection_handler(socketFds* fd, int pos){
    char message[MAX_SIZE];
    int len, err;

    if (fd[pos].type == WORKER) {

        fd_set masterStats, branchStats;
        FD_ZERO(&masterStats);
        FD_SET(fd[pos].fd, &masterStats);

        int i = 0;
        do {
            branchStats = masterStats;
            if((select(FD_SETSIZE, &branchStats, NULL, NULL, NULL)) < 0){ return; }
            for (int j = 0; j < FD_SETSIZE; ++j) {
                if(FD_ISSET(j, &branchStats)){
                    bzero(message, sizeof(message));
                    len = read(j, message, MAX_SIZE);
                    if (len > 0) {
                        message[len] = '\0';
                        if (i == 0) {
                            if(strcmp(message, "end") == 0){
                                printf("Thread %ld has no work to do\n", pthread_self());
                                return;
                            }
                            pthread_mutex_lock(&workerCountMutex);
                            workerQueryPort[workerCount] = atoi(message);

                            workerCount++;
                            pthread_mutex_unlock(&workerCountMutex);

                            i++;
                        } else {

                            pthread_mutex_lock(&printMutex);
                            if (strcmp(message, "done") == 0){
                                printf("#############Thread %ld : HEYYYY I'M %s\n", pthread_self(), message);
                            } else{
                                //printf("%s", message);
                            }
                            pthread_mutex_unlock(&printMutex);

                        }
                    }else{
                        close(j);
                        FD_CLR(j, &masterStats);
                    }
                }
            }
        } while (strcmp(message, "done") != 0);

        fds[pos].fd = -1;
        fds[pos].type = -1;
    } else if( fds[pos].type == CLIENT ) {

        //############# SERVER CONNECT TO WORKER FOR QUERIES #################//

        FD_SET(fd[pos].fd, &masterWorker);
        //printf("Connection_handler : Thread %ld Communicating with whoClient fd %d position %d\n", pthread_self(), fds[pos].fd,pos);
        do {
            branchWorker = masterWorker;
            if((select(FD_SETSIZE, &branchWorker, NULL, NULL, NULL)) < 0){ return; }
            for (int j = 0; j < FD_SETSIZE; ++j) {
                if (FD_ISSET(j, &branchWorker)) {
                    bzero(message, sizeof(message));
                    len = read(j, message, MAX_SIZE);       //read Query from whoClient
                    if (len > 0) {
                        message[len] = '\0';

                        pthread_mutex_lock(&workerSocketMutex);
                        for (int i = 0; i < workerCount; ++i) {
                            write(workerSocket[i], message, sizeof(message));
                        }
                        pthread_mutex_unlock(&workerSocketMutex);

                        for (int i = 0; i < workerCount; ++i) {
                            bzero(message, sizeof(message));
                            int bytesIn = read(workerSocket[i], message, sizeof(message));       //read answer from worker
                            if(bytesIn > 0) {
                                write(j, message, sizeof(message));                               //write answer to client
                                message[bytesIn] = '\0';
                            }
                        }
                        bzero(message, sizeof(message));
                        strcpy(message, "done");
                        write(j, message, sizeof(message));                               //write answer to client

                        pthread_mutex_lock(&printMutex);
                        printf("\nThread %ld is clearing fds (Query)\n", pthread_self());
                        pthread_mutex_unlock(&printMutex);
                        fds[pos].fd = -1;
                        fds[pos].type = -1;

                        return;
                    } else {
                        close(j);
                        FD_CLR(j, &masterWorker);
                    }
                }
            }
        } while (1);
    }
}

void SetUpWorker(){
    pthread_mutex_lock(&printMutex);
    printf("workerCount : %d\n", workerCount);
    pthread_mutex_unlock(&printMutex);

    struct sockaddr_in workerServs[workerCount];
    bzero(workerServs, sizeof(workerServs));

    for (int k = 0; k < workerCount; ++k) {
        workerServs[k].sin_family = AF_INET;
        workerServs[k].sin_addr.s_addr = worker_addr.sin_addr.s_addr;

        workerServs[k].sin_port = workerQueryPort[k];

        if ((workerSocket[k] = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("can't open stream socket");
            exit(1);
        }

        if ((connect(workerSocket[k], (struct sockaddr *) &workerServs[k], sizeof(workerServs[k]))) < 0) {
            perror("can't connect to worker (Query)");
            exit(1);
        }
    }
}