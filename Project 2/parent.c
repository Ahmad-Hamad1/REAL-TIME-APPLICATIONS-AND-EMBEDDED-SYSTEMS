#include "local.h"

void initSharedMemory(sharedMemory *hall);
void readInputsFile();
void endOfProgram(int n);

int continueProgram = 1;
int c;

extern int main() {
    signal(SIGUSR1, endOfProgram);
    readInputsFile();
    
    sharedMemory *mem;
    union semun arg;
    union semun argBus;
    int totalBordersNum =
        palestinianBorderNum + jordanianBorderNum + foreignBorderNum;
    int semid;
    int shmId;
    int busSemID;
    char *shmPtr;
    int msgQueueIds[totalBordersNum];
    int bordersIds[totalBordersNum];
    int busesIds[numberOfBusses];

    static unsigned short start_val[1] = {1};
    static unsigned short bus_start_val[1] = {1};

    if ((shmId = shmget(getpid(), sizeof(mem), IPC_CREAT | 0600)) == -1)
        exit(2);
    if ((shmPtr = (char *)shmat(shmId, 0, 0)) == (char *)-1) exit(3);

    memcpy(shmPtr, (char *)&mem, sizeof(mem));

    sharedMemory *temp = (sharedMemory *)shmPtr;
    initSharedMemory(temp);

    // define the semaphore here
    if ((semid = semget(getpid(), 1, IPC_CREAT | IPC_EXCL | 0660)) != -1 &&
        (busSemID = semget(getpid() + 1, 1, IPC_CREAT | IPC_EXCL | 0660)) !=
            -1) {
        arg.array = start_val;
        argBus.array = bus_start_val;

        if (semctl(semid, 0, SETALL, arg) == -1 ||
            semctl(busSemID, 0, SETALL, argBus) == -1) {
            perror("semctl -- producer -- initialization");
            exit(1);
        }
    }

    FILE *ptr = fopen(msgQueuesFile, "w");
    for (int i = 0; i < totalBordersNum; i++) {
        int id;
        if ((id = msgget(getpid() + i, IPC_CREAT | 0660)) == -1) {
            perror("Queue create");
            exit(1);
        }
        if (i == palestinianBorderNum ||
            i == palestinianBorderNum + jordanianBorderNum)
            fprintf(ptr, "\n%d ", id);
        else
            fprintf(ptr, "%d ", id);
        msgQueueIds[i] = id;
    }

    fclose(ptr);
    char shmid[10];
    char semId[10];
    sprintf(semId, "%d", semid);
    sprintf(shmid, "%d", shmId);
    int idx = 0;

    for (int i = 0; i < totalBordersNum; i++) {
        char queueId[10];
        sprintf(queueId, "%d", msgQueueIds[idx++]);
        int id = fork();
        bordersIds[i] = id;
        if (id == 0) {
            execlp("./border", "border", shmid, semId, queueId, (char *)NULL);
        }
    }

    char busSemIDString[10];
    sprintf(busSemIDString, "%d", busSemID);
    for (int i = 0; i < numberOfBusses; i++) {
        if ((busesIds[i] = fork()) == 0) {
            execlp("./bus", "bus", shmid, semId, busSemIDString, (char *)NULL);
        }
    }

    while (continueProgram) {
        int passengers = (rand() % 5) + 1;
        for (int i = 0; i < passengers; i++) {
            int id = fork();
            c++;
            if (id == 0) {
                execlp("./passenger", "passenger", shmid, semId, (char *)NULL);
                perror("execlp failed");
            }
            sleep(1);
        }
    }
    printf(" --%d--\n", c);
    // Delete Message Queues, Semaphores & Shared Memory.
    for (int i = 0; i < totalBordersNum; i++) {
        kill(bordersIds[i], SIGKILL);
        if (msgctl(msgQueueIds[i], IPC_RMID, (struct msqid_ds *)0) == -1) {
            perror("Queue deletion error\n");
        }
    }
    for (int i = 0; i < numberOfBusses; i++) {
        kill(busesIds[i], SIGKILL);
    }

    printf(" [Number of granted Passengers: %d]\n", temp->Pg);
    printf(" [Number of denied Passengers: %d]\n", temp->Pd);
    printf(" [Number of impatient Passengers: %d]\n", temp->Pu);
    shmdt(shmPtr);

    if (semctl(semid, 0, IPC_RMID, 0) == -1 ||
        semctl(busSemID, 0, IPC_RMID, 0) == -1) {
        perror("Semaphore deletion error\n");
    }
    if (shmctl(shmId, IPC_RMID, (struct shmid_ds *)0) == -1) {
        perror("Shared memory deletion error\n");
    }

    killpg(getpid(), SIGKILL);

    return 0;
}

void initSharedMemory(sharedMemory *hall) {
    hall->Pd = 0;
    hall->Pg = 0;
    hall->Pu = 0;
    hall->front = 0;
    hall->rear = 0;
    hall->max = maxThreshold;
    hall->min = minThreshold;
    hall->size = 0;
}

void endOfProgram(int n) { continueProgram = 0; }
