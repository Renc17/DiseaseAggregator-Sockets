#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>

int fail = 0;
int success = 0;
int numofDir = 0;
char **DirList = NULL;

int main(int argc, char** argv) {


    printf("DiseaseAggregator pid %d\n", getpid());

    char *input_dir = NULL;
    int numWorkers = 0;
    int bufferSize = 0;
    int bytesIn = 0;

    int i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-i") != 0) {
        i++;
    }
    input_dir = argv[i + 1];

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-b") != 0) {
        i++;
    }
    bufferSize = atoi(argv[i + 1]);

    i = 0;
    while (argv[i] != NULL && strcmp(argv[i], "-w") != 0) {
        i++;
    }
    numWorkers = atoi(argv[i + 1]);

    pid_t pid[numWorkers];
    pid_t wait_child_id;
    int read_fd[numWorkers], write_fd[numWorkers];
    int status = 0;

    char FIFO1[20];
    char FIFO2[20];

    for (i = 0; i < numWorkers; i++) {
        if ((pid[i] = fork()) == 0) {
            char b[4];
            sprintf(b, "%d", bufferSize);
            if (0 > (execlp("./Worker", "./Worker", input_dir, b, (char *) NULL))) {
                printf("Exec failed\n");
            }
        }

        sprintf(FIFO1, "read%d", pid[i]);
        sprintf(FIFO2, "write%d", pid[i]);

        if ((mkfifo(FIFO1, 0666) < 0)) {
            perror("can't create fifo");
        }
        if ((mkfifo(FIFO2, 0666) < 0)) {
            perror("can't create fifo");
        }
        if ((read_fd[i] = open(FIFO1, O_RDONLY)) < 0) {
            perror("server: can't open read fifo");
        }
        //printf("Parent reading from pip %d\n", read_fd[i]);

        if ((write_fd[i] = open(FIFO2, O_WRONLY)) < 0) {
            perror("server: can't open write fifo");
        }
        //printf("Parent writing in pipe %d\n", write_fd[i]);

    }

    DIR *d;
    struct dirent *dir;
    char **directory;
    d = opendir(input_dir);
    if (d) {
        directory = malloc(numWorkers * sizeof(char *));
        for (int j = 0; j < numWorkers; j++) {
            directory[j] = NULL;
        }
    } else {
        perror(input_dir);
        return -1;
    }

    i = 0;
    int flag = 0;
    char delim[3];
    strcpy(delim, "\n&");

    DirList = malloc(sizeof(char *) * numofDir);

    while ((dir = readdir(d)) != NULL) {    //read input_dir and deliver the directories to the workers throw the pipe
        if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
            continue;
        } else {
            directory[i] = malloc(strlen(dir->d_name)+1);
            strcpy(directory[i], dir->d_name);
            char *writeDir = malloc(bufferSize);
            memset(writeDir, '\0', sizeof(char )* bufferSize);
            strcpy(writeDir, directory[i]);
            write(write_fd[i], writeDir, bufferSize);
            //printf("Parent : Writing directory %s in pipe %d\n", directory[i], write_fd[i]);
            free(writeDir);
            i++;
            if (i == numWorkers) {
                i = 0;    //if there are less workers then directories start over and flag the event
                flag = 1;
            }
            free(directory[i]);     //free for possible replacement
            directory[i] = NULL;

            numofDir++;
            DirList = realloc(DirList, sizeof(char *) * numofDir);
            DirList[numofDir - 1] = malloc(sizeof(char) * strlen(dir->d_name) + 1);
            strcpy(DirList[numofDir - 1], dir->d_name);
        }
    }

    printf("\n\n");
    if (flag == 1) {      //send to every worker the sign that directories have been distributed
        char end[bufferSize];
        memset(end, '\0', sizeof(char )* bufferSize);
        strcpy(end, "end");
        for (int j = 0; j < numWorkers; j++) {
            write(write_fd[j], end, bufferSize);
            //printf("Parent : Sending end pipe %d\n", write_fd[j]);
        }
    } else {   //if there are more workers then directories
        if (i < numWorkers && i != 0) {
            char end[bufferSize];
            memset(end, '\0', sizeof(char )* bufferSize);
            strcpy(end, "end");
            for (int j = 0; j < i; j++) {          //send the ending message to those who got work to do
                write(write_fd[j], end, bufferSize);
                //printf("Parent : Sending signal ending pipe %d\n", write_fd[j]);
            }
            char nodir[bufferSize];
            memset(nodir, '\0', sizeof(char )* bufferSize);
            strcpy(nodir, "noDir");     //tell the others there is no job for them
            for (int j = i; j < numWorkers; j++) {
                write(write_fd[j], nodir, bufferSize);
                //printf("Parent : Sending signal teminate pipe %d\n", write_fd[j]);
                write_fd[j] = -1;
                read_fd[j] = -1;
            }
        }
    }

    closedir(d);

    while ((wait_child_id = wait(&status)) > 0);

    for (i = 0; i < numWorkers; i++) {   //free everything left
        if (directory[i] != NULL)
            free(directory[i]);
    }
    free(directory);

    for (i = 0; i < numWorkers; ++i) {
        sprintf(FIFO1, "read%d", pid[i]);
        sprintf(FIFO2, "write%d", pid[i]);
        unlink(FIFO1);
        unlink(FIFO2);
        if(read_fd[i] != -1)
            close(read_fd[i]);
        if(write_fd[i] != -1)
            close(write_fd[i]);
    }

}