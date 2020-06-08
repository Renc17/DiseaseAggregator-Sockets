#ifndef _LIST_H
#define _LIST_H

#include "patientRecord.h"

typedef struct records Records;

struct records{
    patientRecord* patient;
    Records* next;
};

typedef struct{
    int count;
    Records* first;
    Records* current;
}RecordList;

RecordList* initList();

Records* findId(RecordList* list, char* recordId);
patientRecord* EnterList(RecordList* list, patientRecord* patient, char* date);

void printList(RecordList* list);

void ListDelete(RecordList* list);

#endif
