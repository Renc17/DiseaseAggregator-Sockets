#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdio.h>
#include "RedBlackTree.h"
#include "List.h"

typedef struct {
    char* name;
    RBT* RBTptr;
    RBT* TNILL;
}Value;

typedef struct Bucket{
    Value* value;
    struct Bucket *nextBacket;
}Bucket;

typedef struct HashTable{
    Bucket *buckets;
    Bucket *last;
}HashTable;

typedef struct {
    char category[10];
    int count;
}topkArray;

HashTable *InitHashTable(int numOfEntries);
int hashFunc(char* hashValue,int entries);
Bucket* newbucket(char* hashValue, patientRecord* patient);
void insertPatientHT(HashTable *h, char* hashValue, int entries, patientRecord *patient);
void printHashTable(HashTable* h, int entries);
void getStatistics(HashTable* h, int entries, char* filename, char *dirname, int push_fd);
void DestroyHashTable(HashTable* h, int entries);


void diseaseFrequency(char *answer, char *disease, char* date1, char* date2, char* country, HashTable* h, int entries);
void topkAgeRanges(char *answer, HashTable* h, int entries, int k, char* country, char* disease, char* date1, char* date2);
void searchPatientRecord(char *answer, char* id, RecordList* list);
void numPatientAdmissions(char *answer, HashTable* h, int entries, char* country, char* disease, char* date1, char* date2);
void numPatientDischarges(char *answer, HashTable* h, int entries, char* country, char* disease, char* date1, char* date2);
void Exit(HashTable* h1, HashTable* h2, int entries, RecordList* list);

void Sort (topkArray *array) ;

#endif
