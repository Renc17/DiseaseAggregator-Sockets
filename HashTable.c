#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "HashTable.h"
#include "List.h"

HashTable *InitHashTable(int numOfEntries){
    HashTable* hashTable = malloc(numOfEntries* sizeof(HashTable));
    for (int i = 0; i < numOfEntries; ++i) {
        hashTable[i].buckets = NULL;
        hashTable[i].last = NULL;
    }
    return hashTable;
}

int hashFunc(char* hashValue,int entries){
    int hash = 0;
    int c;
    while ((c = *hashValue++)){
        hash += c;
    }
    return hash%entries;
}

void insertPatientHT(HashTable *h, char* hashValue, int entries, patientRecord *patient){
    int key = hashFunc(hashValue, entries);
    if(h[key].buckets == NULL){
        h[key].buckets = newbucket(hashValue, patient);
        h[key].last = h[key].buckets;
    }else {
        Bucket *currentBucket = h[key].buckets;
        while (currentBucket != NULL){
            if (strcmp(currentBucket->value->name, hashValue) == 0){
                newEntry(&currentBucket->value->RBTptr, &currentBucket->value->TNILL, patient, patient->recordId);
                return;
            }
            currentBucket = currentBucket->nextBacket;
        }
        h[key].last->nextBacket = newbucket(hashValue, patient);
        h[key].last = h[key].last->nextBacket;
    }
}

Bucket *newbucket(char* hashValue, patientRecord *patient){
    Bucket *bucketptr = malloc(sizeof(Bucket));
    bucketptr->nextBacket = NULL;
    bucketptr->value = malloc(sizeof(Value));


    bucketptr->value->name = malloc(sizeof(char)*strlen(hashValue)+1);
    strcpy(bucketptr->value->name, hashValue);
    bucketptr->value->TNILL = init();
    bucketptr->value->RBTptr = bucketptr->value->TNILL;

    newEntry(&bucketptr->value->RBTptr, &bucketptr->value->TNILL, patient, patient->recordId);

    return bucketptr;
}

void DestroyHashTable(HashTable* h, int entries){
    for (int i = 0; i < entries; ++i) {
        if(h[i].buckets != NULL){
            Bucket *currentBucket = h[i].buckets;
            while(currentBucket != NULL) {
                free(currentBucket->value->name);
                DeleteRBTree(currentBucket->value->RBTptr,currentBucket->value->TNILL);
                deleteTNILL(currentBucket->value->TNILL);

                Bucket *temp = currentBucket;
                currentBucket = currentBucket->nextBacket;

                free(temp->value);
                free(temp);
            }
        }
    }
}

void printHashTable(HashTable* h, int entries){
    for (int i = 0; i < entries; ++i) {
        if(h[i].buckets != NULL){
            Bucket *currentBucket = h[i].buckets;
            printf("Buckets %d : \n", i);
            while(currentBucket != NULL) {
                printf("%s\n", currentBucket->value->name);
                print(currentBucket->value->RBTptr, currentBucket->value->TNILL);
                currentBucket = currentBucket->nextBacket;
            }
            printf("\n");
        }
    }
}

void getStatistics(HashTable* h, int entries, char* filename, char* dirname, int push_fd){

    char statMessage[80];
    memset(statMessage, '\0', sizeof(statMessage));

    char num[10];

    strcat(statMessage, dirname);
    strcat(statMessage, "\n");
    strcat(statMessage, filename);
    strcat(statMessage, "\n");

    write(push_fd, statMessage, sizeof(statMessage));
    memset(statMessage, '\0', sizeof(statMessage));

    int categories[4];
    for (int j = 0; j < 4; ++j) {
        categories[j] = 0;
    }

    for (int i = 0; i < entries; ++i) {
        if(h[i].buckets != NULL){
            Bucket *currentBucket = h[i].buckets;
            while(currentBucket != NULL) {
                strcat(statMessage, currentBucket->value->name);
                //printf("%s\n", currentBucket->value->name);
                strcat(statMessage, "\n");
                write(push_fd, statMessage, sizeof(statMessage));
                memset(statMessage, '\0', sizeof(statMessage));


                statistics(currentBucket->value->RBTptr, currentBucket->value->TNILL, categories);

                strcat(statMessage, "Age range 0-20 years: ");
                sprintf(num, "%d cases\n", categories[0]);
                strcat(statMessage, num);
                 //printf("Age range 0-20 years: %d\n", categories[0]);
                write(push_fd, statMessage, sizeof(statMessage));
                memset(statMessage, '\0', sizeof(statMessage));

                strcat(statMessage, "Age range 21-40 years: ");
                sprintf(num, "%d cases\n", categories[1]);
                strcat(statMessage, num);
                //printf("Age range 21-40 years: %d\n", categories[1]);
                write(push_fd, statMessage, sizeof(statMessage));
                memset(statMessage, '\0', sizeof(statMessage));

                strcat(statMessage, "Age range 41-60 years: ");
                sprintf(num, "%d cases\n", categories[2]);
                strcat(statMessage, num);
                //printf("Age range 41-60 years: %d\n", categories[2]);
                write(push_fd, statMessage, sizeof(statMessage));
                memset(statMessage, '\0', sizeof(statMessage));

                strcat(statMessage, "Age range 60+ years: ");
                sprintf(num, "%d cases\n", categories[3]);
                strcat(statMessage, num);
                //printf("Age range 60+ years: %d\n", categories[2]);
                write(push_fd, statMessage, sizeof(statMessage));
                memset(statMessage, '\0', sizeof(statMessage));

                currentBucket = currentBucket->nextBacket;
            }
        }
    }
}

//########## QUERY ##########//

void listCountries(int push_fd, char** Countries, int numOfCountries, int child_id) {
    char *message = malloc(sizeof(char)*100);
    memset(message, '\0', sizeof(char)*100);
    char id[10];

    char delim[2];
    strcpy(delim, "&");

    sprintf(id, " %d\n", child_id);
    for (int i = 0; i < numOfCountries; i++) {
        strcat(message, Countries[i]);
        strcat(message, id);
    }

    strcat(message, delim);
    write(push_fd, message, strlen(message)+1);
    free(message);
}

void diseaseFrequency(int push_fd, char *disease, char* date1, char* date2, char* country, HashTable* h, int entries){
    char *asw = malloc(sizeof(char)*100);
    memset(asw, '\0', sizeof(char)*100);
    char delim[2];
    strcpy(delim, "&");
    if(country == NULL){
        int key = hashFunc(disease, entries);
        if(h[key].buckets == NULL){
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            return;
        } else {
            Bucket *currentBucket = h[key].buckets;
            while(currentBucket != NULL) {
                if(strcmp(currentBucket->value->name, disease) == 0){
                    int count = 0;
                    countPatients(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count);
                    char c[6];
                    sprintf(c, "%d\n", count);
                    strcat(asw, c);
                    strcat(asw, delim);
                    write(push_fd, asw, strlen(asw)+1);
                    free(asw);
                    return;
                }
                currentBucket = currentBucket->nextBacket;
            }
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            //printf("ERROR : No such disease in records\n");
        }
    } else {
        int key = hashFunc(country, entries);
        if(h[key].buckets == NULL){
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            return;
        } else {
            Bucket *currentBucket = h[key].buckets;
            while(currentBucket != NULL) {
                if(strcmp(currentBucket->value->name, country) == 0){
                    int count = 0;
                    countPatientsByDisease(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count, disease);
                    char c[6];
                    sprintf(c, "%d\n", count);
                    strcat(asw, c);
                    strcat(asw, delim);
                    write(push_fd, asw, strlen(asw)+1);
                    free(asw);
                    return;
                }
                currentBucket = currentBucket->nextBacket;
            }
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            //printf("ERROR : No such country in records\n");
        }
    }
}

void searchPatientRecord(int push_fd, char* id, RecordList* list){
    Records* patient = findId(list, id);
    if(patient != NULL) {
        char delim[2];
        strcpy(delim, "&");
        char *asw = malloc(sizeof(char)*100);
        memset(asw, '\0', sizeof(char)*100);

        strcat(asw, patient->patient->recordId);
        strcat(asw," ");
        strcat(asw,  patient->patient->name);
        strcat(asw," ");
        strcat(asw, patient->patient->surname);
        strcat(asw," ");
        strcat(asw, patient->patient->age);
        strcat(asw," ");
        strcat(asw, patient->patient->disease);
        strcat(asw," ");
        strcat(asw, patient->patient->Date->entryDate);
        strcat(asw," ");
        strcat(asw, patient->patient->Date->exitDate);
        strcat(asw,"\n");
        strcat(asw, delim);

        write(push_fd, asw, strlen(asw)+1);
        free(asw);
    } else{
        char error[7];
        strcpy(error, "Error&");
        write(push_fd, error, strlen(error)+1);
    }
}

void numPatientAdmissions(int push_fd, HashTable* h, int entries, char* country, char* disease, char* date1, char* date2){
    char delim[2];
    strcpy(delim, "&");
    char *asw = malloc(sizeof(char)*100);
    memset(asw, '\0', sizeof(char)*100);

    if(country == NULL){
        for (int i = 0; i < entries; ++i) {
            if(h[i].buckets != NULL){
                Bucket *currentBucket = h[i].buckets;
                while(currentBucket != NULL) {
                    strcat(asw, currentBucket->value->name);
                    int count = 0;
                    countPatientsAdmissionByDisease(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count, disease);
                    currentBucket = currentBucket->nextBacket;
                    char c[7];
                    sprintf(c, " %d\n", count);
                    strcat(asw, c);
                }
            }
        }
        strcat(asw, delim);
        write(push_fd, asw, strlen(asw)+1);
        free(asw);
    } else{
        int key = hashFunc(country, entries);
        if(h[key].buckets == NULL){
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            return;
        } else {
            Bucket *currentBucket = h[key].buckets;
            while(currentBucket != NULL) {
                if(strcmp(currentBucket->value->name, country) == 0){
                    strcat(asw, currentBucket->value->name);
                    int count = 0;
                    countPatientsAdmissionByDisease(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count, disease);
                    char c[7];
                    sprintf(c, " %d\n", count);
                    strcat(asw, c);
                    strcat(asw, delim);
                    write(push_fd, asw, strlen(asw)+1);
                    free(asw);
                    return;
                }
                currentBucket = currentBucket->nextBacket;
            }
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            //printf("ERROR : No such disease in records\n");
        }
    }
}

void numPatientDischarges(int push_fd, HashTable* h, int entries, char* country, char* disease, char* date1, char* date2){
    char *asw = malloc(sizeof(char)*100);
    memset(asw, '\0', sizeof(char)*100);
    char delim[2];
    strcpy(delim, "&");

    if(country == NULL){
        for (int i = 0; i < entries; ++i) {
            if(h[i].buckets != NULL){
                Bucket *currentBucket = h[i].buckets;
                while(currentBucket != NULL) {
                    strcat(asw, currentBucket->value->name);
                    int count = 0;
                    countPatientsDischargeByDisease(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count, disease);
                    currentBucket = currentBucket->nextBacket;
                    char c[7];
                    sprintf(c, " %d\n", count);
                    strcat(asw, c);
                }
            }
        }
        strcat(asw ,delim);
        write(push_fd, asw, strlen(asw)+1);
        free(asw);
    } else{
        int key = hashFunc(country, entries);
        if(h[key].buckets == NULL){
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            //printf("ERROR : No such country found\n");
            return;
        } else {
            Bucket *currentBucket = h[key].buckets;
            while(currentBucket != NULL) {
                if(strcmp(currentBucket->value->name, country) == 0){
                    strcat(asw, currentBucket->value->name);
                    int count = 0;
                    countPatientsDischargeByDisease(currentBucket->value->RBTptr, currentBucket->value->TNILL, date1, date2, &count, disease);
                    char c[7];
                    sprintf(c, " %d\n", count);
                    strcat(asw, c);
                    strcat(asw ,delim);
                    write(push_fd, asw, strlen(asw)+1);
                    free(asw);
                    return;
                }
                currentBucket = currentBucket->nextBacket;
            }
            char error[7];
            strcpy(error, "Error&");
            write(push_fd, error, strlen(error)+1);
            //printf("ERROR : No such disease in records\n");
        }
    }
}

void Exit(HashTable* h1, HashTable* h2, int entries, RecordList* list){
    DestroyHashTable(h1, entries);
    DestroyHashTable(h2, entries);
    ListDelete(list);
}

void topkAgeRanges(int push_fd, HashTable* h, int entries, int k, char* country, char* disease, char* date1, char* date2){
    int categories[4];
    for (int j = 0; j < 4; ++j) {
        categories[j] = 0;
    }

    topkArray *array = malloc(4* sizeof(topkArray));
    strcpy(array[0].category, "0-20 : ");
    strcpy(array[1].category, "21-40 : ");
    strcpy(array[2].category, "41-60 : ");
    strcpy(array[3].category, "60+ : ");

    int key = hashFunc(country, entries);
    if(h[key].buckets != NULL){
        Bucket *currentBucket = h[key].buckets;
        while(currentBucket != NULL) {
            if(strcmp(currentBucket->value->name, country) == 0){
                int count = 0;
                topk(currentBucket->value->RBTptr, currentBucket->value->TNILL, categories, date1, date2, &count, disease);
                for (int j = 0; j < 4; ++j) {
                    array[j].count = categories[j];
                }

                Sort(array);

                char top[10];
                char delim[2];
                strcpy(delim, "&");

                char* answer = malloc(sizeof(char)*100);
                memset(answer, '\0', sizeof(char)*100);

                int result;
                for (int i = 0; i < k; ++i) {
                    result = (array[i].count * 100) / count;
                    sprintf(top, " : %d%%\n", result);
                    strcat(answer, array[i].category);
                    strcat(answer, top);
                }

                strcat(answer, delim);
                write(push_fd, answer, strlen(answer)+1);
                free(answer);
                free(array);
                return;
            }
            currentBucket = currentBucket->nextBacket;
        }
    }
    char error[7];
    strcpy(error, "Error&");
    write(push_fd, error, strlen(error)+1);
}

void Sort (topkArray *array) {
    for (int i = 0; i < 4; i++)
    {
        for (int j = i; j < 4; j++)
        {
            if (array[j].count > array[i].count)
            {
                int tmpCount = array[i].count;
                char tmpValue[10];
                strcpy(tmpValue, array[i].category);

                array[i].count = array[j].count;
                strcpy(array[i].category, array[j].category);

                array[j].count = tmpCount;
                strcpy(array[j].category, tmpValue);
            }
        }
    }
}