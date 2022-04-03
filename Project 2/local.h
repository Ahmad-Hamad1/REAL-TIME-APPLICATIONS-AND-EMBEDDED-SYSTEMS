#include <errno.h>
#include <memory.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

double palestinianRatio;
double jordanianRatio;
double foreignRatio;
int palestinianBorderNum;
int jordanianBorderNum;
int foreignBorderNum;
int numberOfBusses;
int busCapacity;
int accessGranted;
int accessDenied;
int turnedBack;
int minBusRange;
int maxBusRange;
int minThreshold;
int maxThreshold;

char msgQueuesFile[20] = "queuesIds.txt";

typedef struct {
    int passengerPid;
    int validPassport;
} passengerDetails;

typedef struct {
    int Pg;
    int Pd;
    int Pu;
    int hallQueue[1000];
    int front;
    int rear;
    int max;
    int min;
    int size;
} sharedMemory;

typedef struct node {
    int passengerPid;
    int validPassport;
    struct node *next;
} node;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct sembuf acquire = {0, -1, SEM_UNDO}, release = {0, 1, SEM_UNDO};

struct sembuf acquireBus = {0, -1, SEM_UNDO}, releaseBus = {0, 1, SEM_UNDO};

int addToHall(sharedMemory *hall, int passengerId) {
    int front = hall->front;
    int rear = hall->rear;
    int max = hall->max;
    int size = hall->size;

    if (size == max) {
        return 0;
    }
    hall->hallQueue[rear] = passengerId;
    hall->rear = (rear + 1) % max;
    hall->size = size + 1;
    return 1;
}

int removeFromHall(sharedMemory *hall) {
    int front = hall->front;
    int rear = hall->rear;
    int max = hall->max;
    int size = hall->size;

    if (size == 0) {
        return -1;
    }

    int tempPassengerId = hall->hallQueue[front];
    hall->front = (front + 1) % max;
    hall->size = size - 1;
    return tempPassengerId;
}

int isHallFull(sharedMemory *hall) { return (hall->size == hall->max) ? 1 : 0; }

int checkThreshold(sharedMemory *hall) { return hall->size < hall->min; }

void readInputsFile() {
    FILE *filePtr = fopen("inputs.txt", "r");

    fscanf(filePtr, "%d", &palestinianBorderNum);
    fscanf(filePtr, "%d", &jordanianBorderNum);
    fscanf(filePtr, "%d", &foreignBorderNum);

    fscanf(filePtr, "%d", &numberOfBusses);
    fscanf(filePtr, "%d", &busCapacity);
    fscanf(filePtr, "%d", &minBusRange);
    fscanf(filePtr, "%d", &maxBusRange);

    fscanf(filePtr, "%d", &minThreshold);
    fscanf(filePtr, "%d", &maxThreshold);

    fscanf(filePtr, "%lf", &palestinianRatio);
    fscanf(filePtr, "%lf", &jordanianRatio);
    fscanf(filePtr, "%lf", &foreignRatio);

    fscanf(filePtr, "%d", &turnedBack);     // Pu
    fscanf(filePtr, "%d", &accessGranted);  // Pg
    fscanf(filePtr, "%d", &accessDenied);   // Pd

    fclose(filePtr);
}