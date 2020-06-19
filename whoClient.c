#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//./whoClient –q queryFile -w numThreads –sp servPort –sip servIP

void *Client(void *query);

int numThreads = 0;
int servPort = 0;
char *servIP;
int available = 0;

pthread_mutex_t printMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t connectMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;

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
                //printf("Creating %d thread, i : %d\n", j, i);
                if ((err = pthread_create(&thr[j], NULL, Client, (void *) queries[i]))) {
                    printf("Invalid : pthread_create %d\n", err);
                    exit(1);
                }
                i++;
            }
        }

        if ((err = pthread_mutex_lock(&connectMtx))) {
            exit(1);
        }
        available++;

        //printf("whoClient : Waking up my threads\n");
        pthread_cond_signal(&condition);
        if ((err = pthread_mutex_unlock(&connectMtx))) {
            exit(1);
        }

        for (int j = 0; j < numThreads; j++) {
            if(k < i){
                //printf("Waiting for %d thread, k : %d\n", j, k);
                if ((err = pthread_join(*(thr + j), NULL))) {
                    printf("Invalid : pthread_join %d\n", err);
                    exit(1);
                }
                k++;
            }
        }
    }

    printf("\nOriginal thread exiting\n");

    for (int j = 0; j < numQuery; ++j) {
        free(queries[j]);
    }
    free(queries);
    free(thr);

}

void *Client(void *query) {

    int socketFd;
    struct sockaddr_in server;
    int position = 0, pos = 0;
    int err;

    //Setup Server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(servIP);
    server.sin_port = htons(servPort);
    //printf("Client : Port : %d Ip : %d\n", servPort, server.sin_addr.s_addr);

    if ((err = pthread_mutex_lock(&connectMtx))) {
        exit(1);
    }
    //printf("Client %ld : Locked connect mutex\n", pthread_self());

    //printf("Client %ld : Waiting for other thread to create\n", pthread_self());
    if(available == 0) {
        //printf("Thread %ld : Waiting for Condition Available %d\n", pthread_self(), available);
        pthread_cond_wait(&condition, &connectMtx);
        available--;
    }
    /* open a TCP socket */
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    //printf("Client %ld : Waiting to connect to server\n", pthread_self());
    //###################### condition variable needed here ###############################//
    //sleep(2);
    /* connect to the server */
    if ((connect(socketFd, (struct sockaddr *) &server, sizeof(server))) < 0) {
        perror("can't connect to server");
        exit(1);
    }

    //pos = position;
    //position++;

    if ((err = pthread_mutex_unlock(&connectMtx))) {
        exit(1);
    }

    char buf[80];
    int len;

    printf("%s", (char *) query);

    /* write a message to the server */
    char q[80];
    bzero(q, sizeof(q));
    strcpy(q, query);
    write(socketFd, q, sizeof(q));

    int gotAnswer = 1;

    do{
        len = read(socketFd, buf, sizeof(buf));       //read Query from Server
        if (len > 0) {
            buf[len] = '\0';
            if ((err = pthread_mutex_lock(&printMtx))) {
                exit(1);
            }
            if(strcmp(buf, "Error") != 0 && strcmp(buf, "done") != 0){
                printf("%s", buf);
            }
            if ((err = pthread_mutex_unlock(&printMtx))) {
                exit(1);
            }
            if(strcmp(buf, "done") == 0){
                gotAnswer = 0;
            }
            bzero(buf, sizeof(buf));
        } else{
            close(socketFd);
        }
    }while ( gotAnswer );


    printf("\n\nClient is exiting thread\n");
    pthread_exit(NULL);
}
