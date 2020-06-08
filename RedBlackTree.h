#ifndef _REDBLACKTREE_H
#define _REDBLACKTREE_H

#include <stdio.h>
#include "patientRecord.h"

typedef struct RBTnode
{
    patientRecord *patientNode;
    char *primaryKey;
    int color;
    struct RBTnode *left, *right, *parent;
}RBT;

typedef struct {
    int day;
    int month;
    int year;
}DateConvert;

RBT* init();
void LeftRotate(RBT **root,RBT* x, RBT* tn);
void rightRotate(RBT **root,RBT* x, RBT* tn);
void insertFixUp(RBT **root,RBT* z, RBT* tn);
void newEntry(RBT **root, RBT** tn, patientRecord* p, char* primaryKey);
RBT* findInRBT(char* primaryKey, RBT* root, RBT* tn);

int compareDate(DateConvert* date1, DateConvert* date2);
DateConvert convert(char *date);

void countPatients(RBT *root, RBT *tn, char* date1, char* date2, int *count);
void countPatientsByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease);
void countPatientsAdmissionByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease);
void countPatientsDischargeByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease);
void topk(RBT *Root, RBT *tn, int *array, char* date1, char* date2, int *count, char* disease);

void print(RBT* root, RBT* tn, FILE *fd);
void statistics(RBT *Root, RBT *tn, int *Array);

void DeleteRBTree(RBT* node ,RBT* tn);
void deleteTNILL(RBT* tn);

#endif
