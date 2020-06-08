#ifndef _PATIENTRECORD_H
#define _PATIENTRECORD_H

typedef struct Date{
    char *entryDate;
    char *exitDate;
}Date;

typedef struct patient{
    char * recordId;
    char *status;
    char *name;
    char *surname;
    char *age;
    char *disease;
    Date* Date;
}patientRecord;

patientRecord* initialize();

void set_id(patientRecord* v, char* id);
void set_name(patientRecord* v, char* n);
void set_surname(patientRecord* v, char* sn);
void set_disease(patientRecord* v, char* dis);
void set_age(patientRecord* v, char* age);
void set_status(patientRecord* v, char* st);
void set_date(patientRecord* v, char* date, int status);

char* get_id(patientRecord* v);
char* get_name(patientRecord* v);
char* get_surname(patientRecord* v);
char* get_disease(patientRecord* v);
char* get_age(patientRecord* v);
char* get_status(patientRecord* v);

void delete_patient(patientRecord* v);
void deleteDup(patientRecord *v);

#endif
