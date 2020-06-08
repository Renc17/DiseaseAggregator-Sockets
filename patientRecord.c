#include "patientRecord.h"
#include "RedBlackTree.h"
#include <stdlib.h>
#include <string.h>

patientRecord* initialize(){
    patientRecord* v = malloc(sizeof(patientRecord));
    v->Date = malloc(sizeof(Date));
    return v;
}

char* get_id(patientRecord* v){
    return v->recordId;
}

char* get_name(patientRecord* v){
    return v->name;
}

char* get_surname(patientRecord* v){
    return v->surname;
}

char* get_disease(patientRecord* v){
    return v->disease;
}

char* get_age(patientRecord* v){
    return v->age;
}

char* get_status(patientRecord* v){
    return v->status;
}

void set_id(patientRecord* v, char* id){
    strcpy(v->recordId, id);
}

void set_name(patientRecord* v, char* n){
    strcpy(v->name,n);
}

void set_surname(patientRecord* v, char* sn){
    strcpy(v->surname, sn);
}

void set_disease(patientRecord* v, char* dis){
    strcpy(v->disease, dis);
}

void set_age(patientRecord* v, char* age){
    strcpy(v->age, age);
}

void set_status(patientRecord* v, char* st){
    strcpy(v->status, st);
}

void set_date(patientRecord* v, char* date, int status){
    if(status == 0){
        v->Date->entryDate = malloc(sizeof(char)*strlen(date)+1);
        strcpy(v->Date->entryDate, date);
        v->Date->exitDate = malloc(2*sizeof(char));
        strcpy(v->Date->exitDate, "-");
    }else{
        DateConvert entry = convert(v->Date->entryDate);
        DateConvert exit = convert(date);
        if(compareDate(&entry, &exit) <= 0) {
            free(v->Date->exitDate);
            v->Date->exitDate = malloc(sizeof(char) * strlen(date) + 1);
            strcpy(v->Date->exitDate, date);
        }
    }
}

void delete_patient(patientRecord* v){
    free(v->recordId);
    free(v->name);
    free(v->surname);
    free(v->disease);
    free(v->status);
    free(v->age);
    free(v->Date->exitDate);
    free(v->Date->entryDate);
    free(v->Date);
    free(v);
}

void deleteDup(patientRecord *v){
    free(v->recordId);
    free(v->name);
    free(v->surname);
    free(v->disease);
    free(v->status);
    free(v->age);
    free(v->Date);
    free(v);
}