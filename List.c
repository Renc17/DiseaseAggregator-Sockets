#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "List.h"
#include "patientRecord.h"

RecordList* initList(){
    RecordList* list = malloc(sizeof(RecordList));
    list->current = NULL;
    list->first = NULL;
    list->count=0;
    return list;
}

Records* findId(RecordList* list, char* recordId){
    Records* temp = list->first;
    while (temp != NULL){
        if(strcmp(temp->patient->recordId, recordId) == 0){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

patientRecord* EnterList(RecordList* list, patientRecord* patient, char* date){
    if ( strcmp(patient->status, "EXIT") == 0){
        Records* temp = findId(list, patient->recordId);
        if(temp != NULL){
            deleteDup(patient);
            set_date(temp->patient, date, 1);
            return temp->patient;
        } else {
            //fprintf(stderr, "ERROR\n");
            return NULL;
        }
    }

    Records* newRec = malloc(sizeof(Records));
    newRec->patient = patient;
    set_date(newRec->patient, date, 0);
    newRec->next = NULL;
    if(list->first == NULL){ list->first = newRec; list->current = newRec; }
    else {
        list->current->next = newRec;
        list->current = list->current->next;
    }
    list->count++;
    return patient;
}

void printList(RecordList* list){
    Records* temp = list->first;
    while (temp != NULL){
        printf("%s %s %s %s %s %s %s %s\n",temp->patient->recordId, temp->patient->status, temp->patient->name, temp->patient->surname, temp->patient->age, temp->patient->disease, temp->patient->Date->entryDate, temp->patient->Date->exitDate);
        temp = temp->next;
    }
}

void ListDelete(RecordList* list){
    Records* temp = list->first;
    while (temp != NULL){
        list->first = list->first->next;
        delete_patient(temp->patient);
        free(temp);
        temp = list->first;
    }
    free(list);
}