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

int fail = 0;
int success = 0;
int numofDir = 0;
char **DirList = NULL;


void updateStucts(FILE* fd, RecordList *Records, HashTable* RecordsByDisease, HashTable* RecordsByCountry, char *filename, char* dirname, int push, int bufferSize);
void EnterPatientRecord(RecordList* list, HashTable* disease, HashTable* Country, HashTable* stat, int entries, char* date, patientRecord* patient, char* dirName);
void SortFiles (char **FileList, int numOfFiles);
void signal_handler(int sig);
void make_logfile();

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

    RecordList *Records = initList();           //list that stores all the patients

    HashTable *RecordsByDisease = InitHashTable(23);
    HashTable *RecordsByCountry = InitHashTable(23);

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

            updateStucts(fd, Records, RecordsByDisease, RecordsByCountry, FilePerDir[j], DirList[i], writefd, bufferSize);

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

    write(writefd, "done", bufferSize);
    close(writefd);
    close(readfd);

    for (int i = 0; i < numofDir; ++i) {
        free(DirList[i]);
    }
    free(DirList);

    free(buf);
    free(message);


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
        make_logfile();
    } else if(sig == SIGQUIT){
        signal(SIGQUIT, signal_handler);
        printf("Worker got users SIGQUIT\n");
        make_logfile();
    }
}

void make_logfile() {
    chdir("..");
    char name[20];
    sprintf(name, "log_file%d", getpid());
    FILE *logFile = fopen(name, "w+");

    for (int i = 0; i < numofDir; i++) {
        if (DirList[i] != NULL)
            fprintf(logFile, "%s\n", DirList[i]);
    }

    int total = success + fail;
    fprintf(logFile, "TOTAL %d\n", total);
    fprintf(logFile, "SUCCESS %d\n", success);
    fprintf(logFile, "FAIL %d\n", fail);
    fclose(logFile);
}

void updateStucts(FILE* fd, RecordList *Records, HashTable* RecordsByDisease, HashTable* RecordsByCountry, char *filename, char* dirname, int push, int bufferSize){

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
    //getStatistics(Stats, 23, filename, dirname, push, bufferSize);    //categorize and push to pipe the file statistics
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