#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define NUM_OF_LOADING_EMPLOYEES 5

typedef struct {
    int steps[10];  // index + 5.
    int finishedSteps;
    int id;
} Laptop;

typedef struct Node Node;
struct Node {
    Laptop *laptop;
    Node *next;
};

typedef struct {
    int lineNumber;
    int empNumber;
} ThreadParam;

int orderedSteps[10][4];

int profit = 0;
int hrSalary = 0;
int laptopCost = 0;
int laptopPrice = 0;
int ceoSalary = 0;
int truckCapacity = 0;
int techEmpSalary = 0;
int extraEmpsSalary = 0;
int storageEmpSalary = 0;
int loadingEmpSalary = 0;
int profitThreshold = 0;
int minStorageCapacity = 0;
int maxStorageCapacity = 0;
int truckDriversSalary = 0;
int profitToEnd = 0;
int storageEmpMinDelay = 0;
int storageEmpMaxDelay = 0;
int truckMinDelay = 0;
int truckMaxDelay = 0;

int laptopIDS = 0;
int cartonNum[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int soldLaptops = 0;
int suspendLines = 0;
int readyLaptops = 0;
int linesToSuspend = 0;
int storageCount = 0;
int isAddToStorage = 1;
int suspendedLines[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Node *linesLists[10];

pthread_mutex_t locks[10][5];
pthread_mutex_t cartonLocks[10];
pthread_mutex_t suspend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t truckLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t laptopLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t profitLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t storageLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t empsSuspend = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t soldLaptopsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t readyLaptopsLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t addToStorage = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empsSuspendCondition = PTHREAD_COND_INITIALIZER;
pthread_cond_t suspensionOfLinesCondition = PTHREAD_COND_INITIALIZER;

pthread_t HRThread;
pthread_t CEOThread;
pthread_t storageEmps[10];
pthread_t accountantThread;
pthread_t employeesIds[10][10];
pthread_t loadingEmployees[NUM_OF_LOADING_EMPLOYEES];

int ids[] = {0, 1, 2, 3, 4};

void exitProgram();            /* finishing program. */
void addToList(Laptop *, int); /* add new laptop to the unready laptops list. */
void *linesEmployee(void *arg);  /* line employee work thread. */
void *storageEmpWork(void *arg); /* storage employee work thread. */
void *loadingEmpWork(void *arg); /* loading employee work thread. */
void *HR(void *arg);             /* HR work thread. */
void *CEO(void *arg);            /* CEO work thread. */
void *accountant(void *arg);      /* accountant work thread. */
Laptop *getFromList(int, int);   /* get laptop from the unready laptops list. */
Laptop *copyLaptop(Laptop *laptop); /* clone the arg laptop. */

void exitProgram() { /* terminate the program. */
    // destroy all mutexes.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            if (pthread_mutex_destroy(&locks[i][j])) {
                perror("Destroy mutex");
                exit(1);
            }
        }
    }
    if (pthread_mutex_destroy(&truckLock) ||
        pthread_mutex_destroy(&laptopLock) ||
        pthread_mutex_destroy(&storageLock) ||
        pthread_mutex_destroy(&readyLaptopsLock) ||
        pthread_mutex_destroy(&addToStorage)) {
        perror("Destroy mutex");
        exit(1);
    }

    for (int i = 0; i < 10; i++) {
        if (pthread_mutex_destroy(&cartonLocks[i])) {
            perror("Initiate mutex");
            exit(1);
        }
    }

    exit(0);
}

FILE *filePtr;
int getValueOfLine() { /* take the value of one line from input.txt file. */
    char line[101];

    if (fscanf(filePtr, "%s", line) == EOF ||
        fscanf(filePtr, "%s", line) == EOF) {
        perror("Error n reading file");
        exit(1);
    }
    return atoi(line);
}

void readInputsFile() { /* read and store all user-define variables. */
    char fileName[] = "inputs.txt";

    filePtr = fopen(fileName, "r");
    if (filePtr == NULL) {
        perror("file not exist");
        exit(1);
    }

    profit = getValueOfLine();
    profitThreshold = getValueOfLine();
    laptopCost = getValueOfLine();
    laptopPrice = getValueOfLine();
    hrSalary = getValueOfLine();
    ceoSalary = getValueOfLine();
    extraEmpsSalary = getValueOfLine();
    storageEmpSalary = getValueOfLine();
    loadingEmpSalary = getValueOfLine();
    techEmpSalary = getValueOfLine();
    truckDriversSalary = getValueOfLine();
    truckCapacity = getValueOfLine();
    minStorageCapacity = getValueOfLine();
    maxStorageCapacity = getValueOfLine();
    profitToEnd = getValueOfLine();
    storageEmpMinDelay = getValueOfLine();
    storageEmpMaxDelay = getValueOfLine();
    truckMinDelay = getValueOfLine();
    truckMaxDelay = getValueOfLine();

    if (fclose(filePtr)) {
        perror("cannot close file");
        exit(1);
    }
}