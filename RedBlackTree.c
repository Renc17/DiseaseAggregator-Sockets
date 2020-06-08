#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RedBlackTree.h"
#include "patientRecord.h"

RBT* init(){
    RBT* tnill = malloc(sizeof(struct RBTnode));
    tnill -> patientNode = NULL;
    tnill -> color = 0;
    tnill -> left = tnill -> right = tnill -> parent = NULL;
    return tnill;
}

void LeftRotate(RBT **root,RBT* x, RBT* tn){
    RBT* y = x->right;
    x->right = y->left;
    if(y->left != tn){
        y->left->parent = x;
    }
    y->parent = x->parent;
    if(x->parent ==  tn){
        *root = y;
    }else if(x == x->parent->left){
        x->parent->left = y;
    }else{
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}


void rightRotate(RBT **root,RBT* x, RBT* tn){
    RBT* y = x->left;
    x->left = y->right;
    if(y->right != tn){
        y->right->parent = x;
    }
    y->parent = x->parent;
    if(x->parent ==  tn){
        *root = y;
    }else if(x == x->parent->right){
        x->parent->right = y;
    }else{
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

void insertFixUp(RBT **root,RBT* z, RBT* tn){

    while (z != *root && z != (*root)->left && z != (*root)->right && z->parent->color == 1)
    {
        RBT* y;

        // Find uncle and store uncle in y
        if (z->parent != tn && z->parent == z->parent->parent->left)
            y = z->parent->parent->right;
        else
            y = z->parent->parent->left;

        // If uncle is RED, do following
        // (i)  Change color of parent and uncle as BLACK
        // (ii) Change color of grandparent as RED
        // (iii) Move z to grandparent
        if (y->color == 1)
        {
            y->color = 0;
            z->parent->color = 0;
            z->parent->parent->color = 1;
            z = z->parent->parent;
        }

            // Uncle is BLACK, there are four cases (LL, LR, RL and RR)
        else
        {
            // Left-Left (LL) case, do following
            // (i)  Swap color of parent and grandparent
            // (ii) Right Rotate Grandparent
            if (z->parent != tn && z->parent == z->parent->parent->left &&
                z == z->parent->left)
            {
                int ch = z->parent->color ;
                z->parent->color = z->parent->parent->color;
                z->parent->parent->color = ch;
                rightRotate(root,z->parent->parent,tn);
            }

            // Left-Right (LR) case, do following
            // (i)  Swap color of current node  and grandparent
            // (iii) Right Rotate Grand Parent
            if (z->parent != tn && z->parent == z->parent->parent->left &&
                z == z->parent->right)
            {
                int ch = z->color ;
                z->color = z->parent->parent->color;
                z->parent->parent->color = ch;
                rightRotate(root,z->parent->parent, tn);
            }

            // Right-Right (RR) case, do following
            // (i)  Swap color of parent and grandparent
            // (ii) Left Rotate Grandparent
            if (z->parent != tn &&
                z->parent == z->parent->parent->right &&
                z == z->parent->right)
            {
                int ch = z->parent->color ;
                z->parent->color = z->parent->parent->color;
                z->parent->parent->color = ch;
                LeftRotate(root,z->parent->parent, tn);
            }

            // Right-Left (RL) case, do following
            // (i)  Swap color of current node  and grandparent
            // (ii) Right Rotate Parent
            // (iii) Left Rotate Grand Parent
            if (z->parent != tn && z->parent == z->parent->parent->right &&
                z == z->parent->left)
            {
                int ch = z->color ;
                z->color = z->parent->parent->color;
                z->parent->parent->color = ch;
                rightRotate(root,z->parent, tn);
                LeftRotate(root,z->parent, tn);
            }
        }
    }
    (*root)->color = 0; //keep root always black
}

RBT* findInRBT(char* primaryKey, RBT* root, RBT* tn){
    RBT* y;
    RBT* x = root;
    while(x != tn){
        y = x;
        if(strcmp(y->primaryKey, primaryKey) < 0){
            x = x->left;
        }else if(strcmp(y->primaryKey, primaryKey) > 0){
            x = x->right;
        } else {
            return y;
        }
    }
    return NULL;
}

void newEntry(RBT **root, RBT** tn, patientRecord* p, char* primaryKey){
    RBT* patient = findInRBT(primaryKey, *root, *tn);
    if ( patient != NULL){
        return;
    }
    // Allocate memory for new node
    RBT* z = malloc(sizeof(struct RBTnode));
    z->patientNode = p;
    z->primaryKey = malloc(sizeof(char)*strlen(primaryKey)+1);
    strcpy(z->primaryKey, primaryKey);
    z->color = 1;
    z->left = z->right = z->parent = *tn;

    RBT* y = *tn;
    RBT* x = *root;
    while(x != *tn){
        y = x;
        if(strcmp(y->primaryKey, primaryKey) < 0){
            x = x->left;
        }else if(strcmp(y->primaryKey, primaryKey) > 0){
            x = x->right;
        } else {
            return;
        }
    }

    z->parent = y;
    if(y == *tn){
        *root = z;
    }
    else if(strcmp(y->primaryKey, primaryKey) < 0) {
        y->left = z;
    }else
        y->right = z;

    z->left = *tn;
    z->right = *tn;
    z->color = 1;

    insertFixUp(root, z, *tn);
}

void print(RBT* root, RBT* tn, FILE *fd){
    if(root == tn){
        return;
    }
    print(root->left, tn, fd);
    fprintf(fd,"%s %s %s %s %s %s %s %s\n",root->patientNode->recordId, root->patientNode->status, root->patientNode->name, root->patientNode->surname, root->patientNode->age, root->patientNode->disease, root->patientNode->Date->entryDate, root->patientNode->Date->exitDate);
    print(root->right, tn, fd);
}

void countPatients(RBT *root, RBT *tn, char* date1, char* date2, int *count){
    if(root == tn){
        return;
    }
    DateConvert d1 = convert(date1);
    DateConvert d2 = convert(date2);

    DateConvert RBTDate = convert(root->patientNode->Date->entryDate);

    if (compareDate(&(RBTDate), &d1) >= 0 && compareDate(&(RBTDate), &d2) <= 0) {
        (*count)++;
        //printf("%s %s\n", root->patientNode->Date->entryDate, root->patientNode->recordId);
    }
    countPatients(root->left, tn, date1, date2, count);
    countPatients(root->right, tn, date1, date2, count);
}

void countPatientsAdmissionByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease){
    if(root == tn){
        return;
    }
    DateConvert d1 = convert(date1);
    DateConvert d2 = convert(date2);

    DateConvert RBTDate = convert(root->patientNode->Date->entryDate);

    if (compareDate(&RBTDate, &d1) >= 0 && compareDate(&RBTDate, &d2) <= 0
        && strcmp(root->patientNode->disease, disease) == 0 && strcmp(root->patientNode->Date->exitDate, "-") == 0) {
        (*count)++;
        //printf("%s %s\n", root->patientNode->Date->entryDate, root->patientNode->recordId);
    }
    countPatientsAdmissionByDisease(root->left, tn, date1, date2, count, disease);
    countPatientsAdmissionByDisease(root->right, tn, date1, date2, count, disease);
}

void countPatientsByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease){
    if(root == tn){
        return;
    }
    DateConvert d1 = convert(date1);
    DateConvert d2 = convert(date2);

    DateConvert RBTDate = convert(root->patientNode->Date->entryDate);

    if (compareDate(&RBTDate, &d1) >= 0 && compareDate(&RBTDate, &d2) <= 0
        && strcmp(root->patientNode->disease, disease) == 0 ) {
        (*count)++;
        //printf("%s %s\n", root->patientNode->Date->entryDate, root->patientNode->recordId);
    }
    countPatientsByDisease(root->left, tn, date1, date2, count, disease);
    countPatientsByDisease(root->right, tn, date1, date2, count, disease);
}

void countPatientsDischargeByDisease(RBT *root, RBT *tn, char* date1, char* date2, int *count, char* disease){
    if(root == tn){
        return;
    }
    DateConvert d1 = convert(date1);
    DateConvert d2 = convert(date2);

    DateConvert RBTDate = convert(root->patientNode->Date->entryDate);

    if (compareDate(&(RBTDate), &d1) >= 0 && compareDate(&(RBTDate), &d2) <= 0
        && strcmp(root->patientNode->disease, disease) == 0 && strcmp(root->patientNode->Date->exitDate, "-") != 0) {
        (*count)++;
    }
    countPatientsDischargeByDisease(root->left, tn, date1, date2, count, disease);
    countPatientsDischargeByDisease(root->right, tn, date1, date2, count, disease);
}

int compareDate(DateConvert* date1, DateConvert* date2){
    if(date1->year == date2->year){
        if(date1->month == date2->month){
            if(date1->day == date2->day){
                return 0;
            } else if(date1->day > date2->day){
                return 1;
            }else{
                return -1;
            }
        } else if(date1->month > date2->month){
            return 1;
        }else {
            return -1;
        }
    } else if(date1->year > date2->year){
        return 1;
    }else {
        return -1;
    }
}

DateConvert convert(char *date){   //convert date from char to int and store it into a Date struct
    char* token;
    char skip[2] = "-";

    char* d = malloc(sizeof(char)*strlen(date)+1);
    strcpy(d, date);
    token = strtok(d, "\n");
    DateConvert ed;
    token = strtok(token, skip);
    ed.day = atoi(token);
    token = strtok(NULL, skip);
    ed.month = atoi(token);
    token = strtok(NULL, skip);
    ed.year = atoi(token);

    free(d);
    return ed;
}

void DeleteRBTree(RBT* node ,RBT* tn){
    if (node == tn){
        return;
    }

    DeleteRBTree(node->left, tn);
    DeleteRBTree(node->right, tn);

    free(node->primaryKey);
    free(node);
}

void deleteTNILL(RBT* tn){
    free(tn);
}

void statistics(RBT *Root, RBT *tn, int *array){

    if(Root == tn){
        return;
    }

    statistics(Root->left, tn, array);

    if(atoi(Root->patientNode->age) <= 20){
        array[0]++;
    } else if((atoi(Root->patientNode->age) >= 21) && (atoi(Root->patientNode->age) <= 40)){
        array[1]++;
    } else if((atoi(Root->patientNode->age) >= 41) && (atoi(Root->patientNode->age) <= 60)){
        array[2]++;
    } else if(atoi(Root->patientNode->age) > 60){
        array[3]++;
    }
    statistics(Root->right, tn, array);

}

void topk(RBT *Root, RBT *tn, int *array, char* date1, char* date2, int *count, char* disease){

    if(Root == tn){
        return;
    }

    topk(Root->left, tn, array, date1, date2, count, disease);

    DateConvert d1 = convert(date1);
    DateConvert d2= convert(date2);
    DateConvert RBTdate = convert(Root->patientNode->Date->entryDate);

    if(compareDate(&RBTdate, &d1) >= 0 && compareDate(&RBTdate, &d2) <= 0 && strcmp(Root->patientNode->disease, disease) == 0){
        (*count)++;
        if(atoi(Root->patientNode->age) <= 20){
            array[0]++;
        } else if((atoi(Root->patientNode->age) >= 21) && (atoi(Root->patientNode->age) <= 40)){
            array[1]++;
        } else if((atoi(Root->patientNode->age) >= 41) && (atoi(Root->patientNode->age) <= 60)){
            array[2]++;
        } else if(atoi(Root->patientNode->age) > 60){
            array[3]++;
        }
    }

    topk(Root->right, tn, array, date1, date2, count, disease);

}