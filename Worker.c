#include "patientRecord.h"
#include "HashTable.h"
#include "List.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int numofDir = 0;
char **DirList = NULL;

RecordList *Rec = NULL;           //list that stores all the patients

HashTable *RecordsByDisease = NULL;
HashTable *RecordsByCountry = NULL;


void updateStucts(FILE* fd, RecordList *Records, HashTable* RecordsByDisease, HashTable* RecordsByCountry, char *filename, char* dirname, int socketFd);
void EnterPatientRecord(RecordList* list, HashTable* disease, HashTable* Country, HashTable* stat, int entries, char* date, patientRecord* patient, char* dirName);
void SortFiles (char **FileList, int numOfFiles);
void signal_handler(int sig);
void answerQuery(char *query, char** answer);

int main(int argc, char** argv) {

    int readfd, writefd;

    char FIFO1[20];
    sprintf(FIFO1, "read%d", getpid());

    char FIFO2[20];
    sprintf(FIFO2, "write%d", getpid());

    if ((writefd = open(FIFO1, O_WRONLY)) < 0) {
        perror("client: can't open write fifo \n");
    }

    if ((readfd = open(FIFO2, O_RDONLY)) < 0) {
        perror("client: can't open read fifo \n");
    }

    int bufferSize = atoi(argv[2]);
    int serverPort = atoi(argv[3]);  //Statistics Server port
    char *serverIp = argv[4];
    printf("Worker : ServerIp : %s ServerPort : %d\n", serverIp, serverPort);

    char *buf = malloc(sizeof(char) * bufferSize);
    char *message = malloc(sizeof(char) * 512);
    memset(message, '\0', sizeof(char) * 512);
    int bytesIn;
    DirList = malloc(sizeof(char *) * numofDir);

    while (1) {        //read until & sign, compose and store the names of the directories
        bytesIn = read(readfd, buf, bufferSize);
        if(bytesIn > 0) {
            if (strcmp(buf, "end") == 0) {
                break;
            }
            else if (strcmp(buf, "noDir") == 0) {
                printf("No work for me!!! child pid %d\n", getpid());       //worker with no job free allocated memory than teminate
                close(readfd);
                close(writefd);
                unlink(FIFO1);
                unlink(FIFO2);
                free(buf);
                free(DirList);
                return 0;
            } else {
                numofDir++;
                DirList = realloc(DirList, sizeof(char *) * numofDir);
                DirList[numofDir - 1] = malloc(sizeof(char) * strlen(buf) + 1);
                strcpy(DirList[numofDir - 1], buf);
            }
            memset(buf, '\0', sizeof(char) * bufferSize);
        }
    }

    int serverfd, queryfd;
    struct sockaddr_in server, worker;

    /* open a TCP socket */
    if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("can't open stream socket");
        close(writefd);
        close(readfd);

        for (int i = 0; i < numofDir; ++i) {
            free(DirList[i]);
        }
        free(DirList);

        free(buf);
        free(message);
        exit(1);
    }

    //Setup Server
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(serverIp);
    server.sin_port = htons(serverPort);

    if((connect(serverfd, (struct sockaddr *) &server, sizeof(server))) < 0) {
        perror("can't connect to server");
        close(writefd);
        close(readfd);

        for (int i = 0; i < numofDir; ++i) {
            free(DirList[i]);
        }
        free(DirList);

        free(buf);
        free(message);
        exit(1);
    }

    /* Create socket */
    if (((queryfd = socket(AF_INET, SOCK_STREAM, 0))) < 0) {
        perror("socket"); exit(1); }

    /* Setup my address */
    worker.sin_family = AF_INET;
    worker.sin_addr.s_addr = inet_addr("127.0.0.1");
    worker.sin_port = htons(7000);

    char queryPort[5];
    memset(queryPort, '\0', sizeof(queryPort));
    sprintf(queryPort, "%d", worker.sin_port);
    write(serverfd, queryPort, sizeof(queryPort));

    Rec = initList();           //list that stores all the patients

    RecordsByDisease = InitHashTable(23);
    RecordsByCountry = InitHashTable(23);

    DIR *d;
    struct dirent *dir;

    d = opendir(argv[1]);
    if(!d){
        printf("child error\n");
        return -1;
    }

    int numofFilesPerDir = 0;
    char **FilePerDir = malloc(sizeof(char *) * numofFilesPerDir);

    chdir(argv[1]);
    closedir(d);
    for(int i=0; i<numofDir; i++) {     //for every directory read all the files, store and sort them
        numofFilesPerDir = 0;
        d = opendir(DirList[i]);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
                    continue;
                }
                numofFilesPerDir++;
                FilePerDir = realloc(FilePerDir, sizeof(char *) * numofFilesPerDir);
                FilePerDir[numofFilesPerDir - 1] = malloc(sizeof(char) * strlen(dir->d_name) + 1);
                strcpy(FilePerDir[numofFilesPerDir - 1], dir->d_name);

            }
        }

        SortFiles(FilePerDir, numofFilesPerDir);

        chdir(DirList[i]);

        for (int j = 0; j < numofFilesPerDir; ++j) {      //for every file read and store in structures(list, hashtable, RBT)
            FILE *fd = fopen(FilePerDir[j], "r");
            if (fd == NULL) {
                printf("ERROR : Could not open file\n");
                return -1;
            }

            updateStucts(fd, Rec, RecordsByDisease, RecordsByCountry, FilePerDir[j], DirList[i], serverfd);

            fclose(fd);

            int fdCheck = open(FilePerDir[j], O_WRONLY);        //write & in the end of file to sign that the file is already in structures
            lseek(fdCheck, -sizeof(char), SEEK_END);
            write(fdCheck, "&", sizeof(char));

            close(fdCheck);
            free(FilePerDir[j]);
        }
        chdir("..");
        closedir(d);
    }
    free(FilePerDir);

    char done[10];
    memset(done, '\0', sizeof(done));
    strcpy(done, "done");
    write(serverfd, done, sizeof(done));        //write to statistics port*/

    //Bind queryfd to worker address
    if ((bind(queryfd, (struct sockaddr *) &worker, sizeof(worker))) < 0) {
        perror("bind"); exit(1); }

    listen(queryfd, 1);

    printf("Worker : Waiting for server to port %d\n", worker.sin_port);
    int queryServLen = sizeof(server);
    int newQueryFd = accept(queryfd, (struct sockaddr *) &server, &queryServLen);


    char delim[2];
    memset(delim, '\0', sizeof(delim));
    strcpy(delim, "&");

    char *ans = malloc(sizeof(char )*100);
    memset(ans, '\0', sizeof(char )*100);

    int len;
    char query[80];
    printf("Worker Communicating with server\n");
    do {
        bzero(query, sizeof(query));
        len = read(newQueryFd, query, sizeof(query));       //read Query from Server
        /* make sure it's a proper string */
        if (len != 0) {
            query[len] = '\0';
            printf("Worker got query %s", query);

            answerQuery(query,  &ans);

            //Send Query to worker
            write(newQueryFd, ans, strlen(ans));        //write to Server the answer
            break;
        }
    } while (1);

    free(ans);
    close(queryfd);
    close(newQueryFd);
    //close(serverfd);

    Exit(RecordsByDisease,RecordsByCountry, 23, Rec);
    free(RecordsByCountry);
    free(RecordsByDisease);
    close(writefd);
    close(readfd);

    for (int i = 0; i < numofDir; ++i) {
        free(DirList[i]);
    }
    free(DirList);

    free(buf);
    free(message);

    return 0;
}


void EnterPatientRecord(RecordList* list, HashTable* disease, HashTable* Country, HashTable* stat, int entries, char* date, patientRecord* patient,
                        char* dirName){
    patientRecord* p = EnterList(list, patient, date);
    if(p != NULL){
        insertPatientHT(disease, p->disease, entries, p);
        insertPatientHT(Country, dirName, entries, p);
        insertPatientHT(stat, p->disease, entries, p);
    }
}

void signal_handler(int sig){
    if(sig == SIGINT){
        signal(SIGINT, signal_handler);
        printf("Worker got users SIGINT\n");

    }
}

void updateStucts(FILE* fd, RecordList *Records, HashTable* RecordsByDisease, HashTable* RecordsByCountry, char *filename, char* dirname, int socketFd){

    size_t lineSize = 200;
    char *token;
    char skip[2] = " ";
    char *buffer = malloc(sizeof(char) * lineSize);

    HashTable *Stats = InitHashTable(23);

    while (getline(&buffer, &lineSize, fd) != -1) {

        patientRecord *patient = initialize();

        strtok(buffer, "\n");

        token = strtok(buffer, skip);
        patient->recordId = malloc(sizeof(char) * strlen(token) + 1);
        set_id(patient, token);

        token = strtok(NULL, skip);
        patient->status = malloc(sizeof(char) * strlen(token) + 1);
        set_status(patient, token);

        token = strtok(NULL, skip);
        patient->name = malloc(sizeof(char) * strlen(token) + 1);
        set_name(patient, token);

        token = strtok(NULL, skip);
        patient->surname = malloc(sizeof(char) * strlen(token) + 1);
        set_surname(patient, token);

        token = strtok(NULL, skip);
        patient->disease = malloc(sizeof(char) * strlen(token) + 1);
        set_disease(patient, token);

        token = strtok(NULL, skip);
        patient->age = malloc(sizeof(char) * strlen(token) + 1);
        set_age(patient, token);

        EnterPatientRecord(Records, RecordsByDisease, RecordsByCountry, Stats, 23, filename, patient, dirname);
    }
    //getStatistics(Stats, 23, filename, dirname, socketFd);    //categorize and push to pipe the file statistics
    DestroyHashTable(Stats, 23);
    free(Stats);
    free(buffer);
}

void SortFiles (char **FileList, int numOfFiles) {

    for (int i = 0; i < numOfFiles; i++)
    {
        for (int j = i+1; j < numOfFiles; j++)
        {
            DateConvert d1 = convert(FileList[i]);
            DateConvert d2 = convert(FileList[j]);
            if (compareDate(&d1, &d2) > 0)
            {
                char *tmpValue = malloc(strlen(FileList[i])* sizeof(char)+1);
                strcpy(tmpValue, FileList[i]);

                strcpy(FileList[i], FileList[j]);

                strcpy(FileList[j], tmpValue);
                free(tmpValue);
            }
        }
    }
}


void answerQuery(char *query, char** answer){
    char *token;
    char skip[2] = " ";

    strcat(query, "\n");
    strtok(query, "\n");
    token = strtok(query, skip);
    if (strcmp(token, "/diseaseFrequency") == 0) {
        token = strtok(NULL, skip);
        if (token == NULL) {
            strcpy(*answer, "Error");
            return;
        }
        char *virusName = malloc(strlen(token) + 1);
        strcpy(virusName, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Entry Date must be entered\n");
            free(virusName);
            return;
        }
        char *date1 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date1, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Entry Date must be entered\n");
            free(virusName);
            free(date1);
            return;
        }
        char *date2 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date2, token);

        token = strtok(NULL, skip);
        if (token != NULL) {
            char *country = malloc(sizeof(char) * strlen(token) + 1);
            strcpy(country, token);
            diseaseFrequency(*answer, virusName, date1, date2, country, RecordsByCountry, 23);
            free(country);
            free(virusName);
            free(date1);
            free(date2);
            return;
        } else {
            diseaseFrequency(*answer, virusName, date1, date2, NULL, RecordsByDisease, 23);
            free(virusName);
            free(date1);
            free(date2);
            return;
        }
    }else if (strcmp(token, "/topkAgeRanges") == 0) {
        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Enter rank number\n");
            return;
        }
        int k = atoi(token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Enter Country\n");
            return;
        }
        char *country = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(country, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Enter Disease\n");
            return;
        }
        char *disease = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(disease, token);

        token = strtok(NULL, skip);
        if (token != NULL) {
            char *date1 = malloc(sizeof(char) * strlen(token) + 1);
            strcpy(date1, token);
            token = strtok(NULL, skip);
            if (token == NULL) {
                char error[7];
                strcpy(*answer, "Error");
                free(date1);
                free(country);
                return;
            }
            char *date2 = malloc(sizeof(char) * strlen(token) + 1);
            strcpy(date2, token);
            topkAgeRanges(*answer, RecordsByCountry, 23, k, country, disease, date1, date2);
            free(date1);
            free(date2);
            free(country);
            free(disease);
            return;
        }
    } else if (strcmp(token, "/searchPatientRecord") == 0) {
        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : RecordId must be entered\n");
            return;
        }
        searchPatientRecord(*answer, token, Rec);
        return;
    } else if (strcmp(token, "/numPatientDischarges") == 0) {
        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Enter Disease\n");
            return;
        }
        char *disease = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(disease, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            free(disease);
            //printf("Input Error : Enter Date\n");
            return;
        }
        char *date1 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date1, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            free(disease);
            free(date1);
            //printf("Input Error : Enter Date\n");
            return;
        }
        char *date2 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date2, token);
        token = strtok(NULL, skip);

        if (token == NULL) {
            numPatientDischarges(*answer, RecordsByCountry, 23, NULL, disease, date1, date2);
            free(disease);
            free(date1);
            free(date2);
            return;
        }else {
            char *country = malloc(sizeof(char) * strlen(token) + 1);
            strcpy(country, token);

            numPatientDischarges(*answer, RecordsByCountry, 23, country, disease, date1, date2);
            free(disease);
            free(country);
            free(date1);
            free(date2);
            return;
            }
    } else if (strcmp(token, "/numPatientAdmissions") == 0) {
        token = strtok(NULL, skip);
        if (token == NULL) {
            char error[7];
            strcpy(*answer, "Error");
            //printf("Input Error : Enter Disease\n");
            return;
        }
        char *disease = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(disease, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            //printf("Input Error : Enter Date\n");
            char error[7];
            strcpy(*answer, "Error");
            free(disease);
            return;
        }
        char *date1 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date1, token);

        token = strtok(NULL, skip);
        if (token == NULL) {
            //printf("Input Error : Enter Date\n");
            char error[7];
            strcpy(*answer, "Error");
            free(disease);
            free(date1);
            return;
        }
        char *date2 = malloc(sizeof(char) * strlen(token) + 1);
        strcpy(date2, token);

        token = strtok(NULL, skip);

        if (token == NULL) {
            numPatientAdmissions(*answer, RecordsByCountry, 23, NULL, disease, date1, date2);
            free(disease);
            free(date1);
            free(date2);
            return;
        } else {
            char *country = malloc(sizeof(char) * strlen(token) + 1);
            strcpy(country, token);

            numPatientAdmissions(*answer, RecordsByCountry, 23, country, disease, date1, date2);
            free(disease);
            free(country);
            free(date1);
            free(date2);
            return;
        }
    } else {
        char error[7];
        strcpy(*answer, "Error");
        return;
    }
}