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

int servPort = 0;
char *servIP;

int main(int argc, char** argv){

    char* queryFile = NULL;
    int numThreads = 0;

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
        printf("Buffer %s", buffer);
    }
    fclose(fd);

    i = 0;
    int k = 0;
    //while (i < numQuery) {
        for (int j = 0; j < numThreads; ++j) {
            //if (i < numQuery) {
                printf("Creating %d thread, i : %d\n", j, i);
                if ((err = pthread_create(&thr[j], NULL, Client, (void *) queries[i]))) {
                    printf("Invalid : pthread_create %d\n", err);
                    exit(1);
                }
             //   i++;
            //}
        }
        for (int j = 0; j < numThreads; j++) {
            //if(k < i){
                printf("Waiting for %d thread, k : %d\n", j, k);
                if ((err = pthread_join(*(thr + j), NULL))) {
                    printf("Invalid : pthread_join %d\n", err);
                    exit(1);
                }
             //   k++;
           // }
        }
   // }

    printf("Original thread exiting\n");

    for (int j = 0; j < numQuery; ++j) {
        free(queries[j]);
    }
    free(queries);
    free(thr);

}

void *Client(void *query) {

    printf("%ld Query : %s", pthread_self(), (char *) query);

    int socketFd;
    struct sockaddr_in server;

    //Setup Server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(servIP);
    server.sin_port = htons(servPort);
    printf("Client : Port : %d Ip : %d\n", servPort, server.sin_addr.s_addr);

    /* open a TCP socket */
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    printf("Client : Waiting to connect to server\n");
    /* connect to the server */
    if ((connect(socketFd, (struct sockaddr *) &server, sizeof(server))) < 0) {
        perror("can't connect to server");
        exit(1);
    }

    /* write a message to the server */
    char q[80];
    bzero(q, sizeof(q));
    strcpy(q, query);
    write(socketFd, q, sizeof(q));

    char buf[80];
    printf("whoClient Communicating with server\n");

    int len;
    while ((len = read(socketFd, buf, sizeof(buf))) >= 0){
        if (len != 0) {
            buf[len] = '\0';
            printf("Answer : %s\n", buf);
            bzero(buf, sizeof(buf));
        }
    }

    close(socketFd);
    pthread_exit(NULL);
}
