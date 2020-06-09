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
    while (i < numQuery) {
        for (int j = 0; j < numThreads; ++j) {
            if (i < numQuery) {
                printf("Creating %d thread, i : %d\n", j, i);
                if ((err = pthread_create(&thr[j], NULL, Client, (void *) queries[i]))) {
                    printf("Invalid : pthread_create %d\n", err);
                    exit(1);
                }
                i++;
            }
        }
        for (int j = 0; j < numThreads; j++) {
            if(k < i){
                printf("Waiting for %d thread, k : %d\n", j, k);
                if ((err = pthread_join(*(thr + j), NULL))) {
                    printf("Invalid : pthread_join %d\n", err);
                    exit(1);
                }
                k++;
            }
        }
    }

    printf("Original thread exiting\n");

    for (int j = 0; j < numQuery; ++j) {
        free(queries[j]);
    }
    free(queries);
    free(thr);

}

void *Client(void *query){

    printf("%ld : Query %s", pthread_self(),(char *) query);

//    struct hostent* serverHost;
//    struct in_addr myaddress;
//    inet_aton(servIP, &myaddress);
//    serverHost=gethostbyaddr((const char*)&myaddress, sizeof(myaddress), AF_INET);
//
//    if (serverHost!=NULL){
//        printf("IP-address: Resolved to: %s\n", serverHost->h_name);
//    } else{
//        printf("IP-address: could not be resolved\n");
//        exit(1);
//    }
//
//    if(serverHost->h_addrtype !=  AF_INET) {
//        perror("unknown address type");
//        exit(1);
//    }

    int sockfd;
    struct sockaddr_in server, client;

    //Setup Server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(servIP);
    server.sin_port = htons(servPort);

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); exit(1); }

    /* Setup my address */
    client.sin_family = AF_INET;        /* Internet domain */
    client.sin_addr.s_addr=htonl(INADDR_ANY); /*Any address*/
    client.sin_port = htons(0);         /* Autoselect port */

    if (bind(sockfd, (struct sockaddr *) &client, sizeof(client)) < 0) {
        perror("bind"); exit(1); }

    /* open a TCP socket */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    /* connect to the server */
    if(connect(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't connect to server");
        exit(1);
    }

    /* write a message to the server */
    write(sockfd, query, strlen(query)+1);
    //char *buf = malloc(sizeof(char )*50);
    //if (recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL) < 0) {
    //    perror("recvfrom"); exit(1); }    /* Receive message */
    //printf("%s\n", buf);
    //free(buf);

    close(sockfd);
    pthread_exit(NULL);
}
