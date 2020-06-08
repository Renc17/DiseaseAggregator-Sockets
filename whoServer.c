#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_SIZE 80

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


    int sockfd, newsockfd, clilen;
    struct sockaddr_in client, server;
    int port;
    char string[MAX_SIZE];
    int len;

    /* command line: server [port_number] */

    port = queryPortNum;

    /* open a TCP socket (an Internet stream socket) */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        exit(1);
    }

    /* bind the local address, so that the client can send to server */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("can't bind local address");
        exit(1);
    }

    /* listen to the socket */
    listen(sockfd, numThreads);

    for(;;) {

        /* wait for a connection from a client; this is an iterative server */
        clilen = sizeof(client);
        newsockfd = accept(sockfd, (struct sockaddr *) &client, &clilen);

        if(newsockfd < 0) {
            perror("can't bind local address");
        }

        /* read a message from the client */
        len = read(newsockfd, string, MAX_SIZE);
        /* make sure it's a proper string */
        string[len] = 0;
        if(strcmp(string, "done") == 0)
            break;
        printf("%s\n", string);
        if (sendto(newsockfd, "done", 5, 0, (struct sockaddr *) &client, sizeof(client))<0)
            exit(1);
        close(newsockfd);
    }
    return 0;
}