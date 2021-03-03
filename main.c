#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <fcntl.h>

typedef enum {false, true} bool;

char *read_file(char *filename);

void *pipe_write(int fd[], char *string);

void *pipe_read(int fd[]);

void *pipe_write_t(char *string);

bool is_last();

pthread_mutex_t mutexRead;
pthread_mutex_t mutexWrite;

int FD[2];
int iteration = 0;
int maxIteration = 0;

int main(int argc, char *argv[]) {
     maxIteration = argc - 1;
    pthread_mutex_init(&mutexRead,NULL);
    pthread_mutex_init(&mutexWrite, NULL);

    if (pipe(FD) < 0) {
        perror("ERR : creating pipe");
        return EXIT_FAILURE;
    }

    pid_t processID;

    pthread_t thread[maxIteration];

    if ((processID = fork()) == 0) {
        //child process
        for (int i = 0; i < maxIteration; i++) {
            char *string = read_file(argv[i + 1]);
            pthread_create(thread + i, NULL, (void *(*)(void *)) pipe_write_t, string);
        }
        for (int i = 0; i < maxIteration; i++) {
            pthread_join(*(thread + i), NULL);
        }

    } else {
        //main process
        for (int i = 0; i < maxIteration; i++) {
            pthread_create(thread, NULL, (void *(*)(void *)) pipe_read, FD);
        }
        for (int i = 0; i < maxIteration; i++) {
            pthread_join(*thread, NULL);
        }
        wait(NULL);
    }
    return EXIT_SUCCESS;
}


char *read_file(char *filename) {

    char *buffer = NULL;
    int strSize, read_size;
    FILE *pIoFile = fopen(filename, "r");

    if (pIoFile) {
        fseek(pIoFile, 0, SEEK_END);
        strSize = ftell(pIoFile);

        rewind(pIoFile);

        buffer = (char *) malloc(sizeof(char) * (strSize + 1));
        read_size = fread(buffer, sizeof(char), strSize, pIoFile);

        buffer[strSize] = '\0';

        if (strSize != read_size) {
            free(buffer);
            buffer = NULL;
        }
        fclose(pIoFile);
    }
    return buffer;
}

void *pipe_write(int fd[], char *string) {
    pthread_mutex_lock(&mutexWrite);

    close(fd[0]);
    size_t strSize = strlen(string);

    if (write(fd[1], &strSize, sizeof(size_t)) < 0) {
        perror("couldn't write string size to pip");
    }
    if (write(fd[1], string, sizeof(char) * strSize) < 0) {
        perror("couldn't write to pip");
    }
//    close(FD[1]);
    pthread_mutex_unlock(&mutexWrite);
}

void *pipe_read(int fd[]) {
    pthread_mutex_lock(&mutexRead);
    close(fd[1]);
    size_t sizeString;

    if (read(fd[0], &sizeString, sizeof(size_t)) < 0) {
        perror("Err : reading size_t\n");
    }

    char *string = malloc(sizeString);
    if (read(fd[0], string, sizeof(char) * sizeString) < 0) {
        perror("Err : reading string\n");
    }
    string[sizeString] = '\0';
    printf("%s\n", string);
    free(string);
    if (is_last()){
        close(fd[0]);
    }
    pthread_mutex_unlock(&mutexRead);

}

void *pipe_write_t(char *string) {
    pipe_write(FD, string);
}

bool is_last(){
    if (iteration == maxIteration){
        (iteration)++;
        return true;
    }
    return false;
}
