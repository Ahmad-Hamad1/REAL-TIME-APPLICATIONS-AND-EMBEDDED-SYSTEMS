#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

int writeNew = 0;

void proceedNext(int signal);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("Missing arguments !");
        exit(-1);
    }
    signal(SIGUSR1, proceedNext);
    int childNum = atoi(argv[1]);
    srand((unsigned int) getpid());
    char fileName[50];
    sprintf(fileName, "child%d.txt", childNum);
    while (1) {
        // waiting for a signal from the parent to start printing the numbers
        while (writeNew == 0);
        FILE *filePtr = fopen(fileName, "a");
        if (filePtr == NULL) {
            perror("Filed to open the file");
            exit(-8);
        }
        for (int i = 0; i < 10; i++) {
            fprintf(filePtr, "%d\n", (rand() % 100) + 1);
        }
        writeNew = 0;
        fclose(filePtr);
        if (childNum == 1)
            kill(getppid(), SIGINT);
        else
            kill(getppid(), SIGQUIT);
    }
    return 0;
}

// signal handler for the signal coming from the parent
void proceedNext(int signal) {
    writeNew = 1;
}

