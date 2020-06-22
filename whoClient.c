#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connectMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t getService = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

//./whoClient –q queryFile -w numThreads –sp servPort –sip servIP

void *Client(void *query);

int numThreads = 0;
int servPort = 0;
char *servIP;
int available = 0;

int main(int argc, char** argv){

    char* queryFile = NULL;

    int i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-q") != 0) {
        i++;
    }
    queryFile = argv[i + 1];

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-w") != 0) {
        i++;
    }
    numThreads = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-sp") != 0) {
        i++;
    }
    servPort = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-sip") != 0) {
        i++;
    }
    servIP = argv[i+1];


    size_t lineSize = 100;
    char* buffer = malloc(sizeof(char) * lineSize);

    int numQuery = 0;
    char **queries = malloc(sizeof(char *)*numQuery);

    int err;
    pthread_t *thr;

    if ((thr = malloc(numThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }


    FILE *fd = fopen(queryFile, "r");
    if (fd == NULL) {
        printf("ERROR : Could not open file\n");
        return -1;
    }

    while (getline(&buffer, &lineSize, fd) != -1) {
        numQuery++;
        queries = realloc(queries, sizeof(char *) * numQuery);
        queries[numQuery - 1] = malloc(sizeof(char) * strlen(buffer) + 1);
        strcpy(queries[numQuery - 1], buffer);
    }
    fclose(fd);

    i = 0;
    int k = 0;
    while (i < numQuery) {
        for (int j = 0; j < numThreads; ++j) {
            if (i < numQuery) {
                if ((err = pthread_create(&thr[j], NULL, Client, (void *) queries[i]))) {
                    printf("Invalid : pthread_create %d\n", err);
                    exit(1);
                }
                i++;
            }
        }

        pthread_mutex_lock(&connectMutex);
        available++;
        pthread_cond_signal(&condition);
        pthread_mutex_unlock(&connectMutex);

        for (int j = 0; j < numThreads; j++) {
            if(k < i){
                if ((err = pthread_join(*(thr + j), NULL))) {
                    printf("Invalid : pthread_join %d\n", err);
                    exit(1);
                }
                k++;
            }
        }
    }

    pthread_mutex_lock(&printMutex);
    printf("\nOriginal thread exiting\n");
    pthread_mutex_unlock(&printMutex);

    for (int j = 0; j < numQuery; ++j) {
        free(queries[j]);
    }
    free(queries);
    free(thr);

}

void *Client(void *query) {

    int socketFd;
    struct sockaddr_in server;

    //Setup Server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(servIP);
    server.sin_port = htons(servPort);

    pthread_mutex_lock(&connectMutex);
    if(available == 0) {
        pthread_cond_wait(&condition, &connectMutex);
        available--;
    }

    /* open a TCP socket */
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    sleep(1);

    /* connect to the server */
    if ((connect(socketFd, (struct sockaddr *) &server, sizeof(server))) < 0) {
        perror("can't connect to server");
        exit(1);
    }
    pthread_mutex_unlock(&connectMutex);

    char buf[80];
    int len;

    pthread_mutex_lock(&getService);

    pthread_mutex_lock(&printMutex);
    printf("%s", (char *) query);
    pthread_mutex_unlock(&printMutex);

    /* write a message to the server */
    char q[80];
    bzero(q, sizeof(q));
    strcpy(q, query);
    write(socketFd, q, sizeof(q));

    int gotAnswer = 1;

    fd_set master;
    FD_ZERO(&master);
    FD_SET(socketFd, &master);

    do {
        if((select(FD_SETSIZE, &master, NULL, NULL, NULL)) < 0){ exit(1); }
        for (int i = 0; i < FD_SETSIZE; ++i) {
            if(FD_ISSET(i, &master)){
                len = read(i, buf, sizeof(buf));       //read Query from Server
                if (len > 0) {
                    buf[len] = '\0';
                    pthread_mutex_lock(&printMutex);
                    if(strcmp(buf, "Error") != 0 && strcmp(buf, "done") != 0){
                        printf("%s\n", buf);
                    }
                    pthread_mutex_unlock(&printMutex);

                    if(strcmp(buf, "done") == 0){
                        gotAnswer = 0;
                    }
                    bzero(buf, sizeof(buf));
                } else{
                    close(i);
                    FD_CLR(i, &master);
                }
            }
        }
    } while (gotAnswer);
    pthread_mutex_unlock(&getService);

    pthread_exit(NULL);
}
