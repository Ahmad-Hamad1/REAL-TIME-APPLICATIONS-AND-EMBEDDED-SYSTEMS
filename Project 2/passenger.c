#include "local.h"

void goToTheBus(int n);
void moveOnHandler(int n);
void passportExpiredHandler(int n);
void checkPatienceLevel(sharedMemory *hall);

char *shHallPtr;
int type;  // 1 Palestinian, 2 Jordanian & 3 Foreigner.
int passportExpired;
int patienceLevel;
int state = 0;  // 0 Initial, 1 At Border's Queue, 2 Hall & 3 Bus.
int semId;

int main(int argc, char const *argv[]) {
    srand(getpid());
    signal(SIGCONT, goToTheBus);
    signal(SIGUSR1, moveOnHandler);
    signal(SIGUSR2, passportExpiredHandler);

    readInputsFile();

    int temp = (rand() % 100) + 1;
    if (temp <= palestinianRatio * 100) {
        type = 1;
    } else if (temp <= (jordanianRatio + palestinianRatio) * 100) {
        type = 2;
    } else {
        type = 3;
    }

    passportExpired = rand() % 100;
    patienceLevel = (rand() % 45) + 17;
    int numberOfAvailableOfficers = type == 1   ? palestinianBorderNum
                                    : type == 2 ? jordanianBorderNum
                                                : foreignBorderNum;
    int queuesIds[numberOfAvailableOfficers];
    int shHallId = atoi(argv[1]);
    semId = atoi(argv[2]);
    FILE *queusIdsFile = fopen(msgQueuesFile, "r");

    if (queusIdsFile == NULL) {
        perror("Error Opening Queues IDs File");
        exit(1);
    }

    if ((shHallPtr = (char *)shmat(shHallId, 0, 0)) == (char *)-1) {
        perror("Error Getting Shared Memory");
        exit(2);
    }

    for (int j = 0; j < palestinianBorderNum; j++) {
        int ID;
        fscanf(queusIdsFile, "%d", &ID);
        if (type == 1) queuesIds[j] = ID;
    }
    for (int j = 0; j < jordanianBorderNum; j++) {
        int ID;
        fscanf(queusIdsFile, "%d", &ID);
        if (type == 2) queuesIds[j] = ID;
    }
    for (int j = 0; j < foreignBorderNum; j++) {
        int ID;
        fscanf(queusIdsFile, "%d", &ID);
        if (type == 3) queuesIds[j] = ID;
    }

    fclose(queusIdsFile);
    int chosenBorder = rand() % numberOfAvailableOfficers;
    char pass[101];
    if (type == 1) {
        strcpy(pass, "Pelestinian");
    } else if (type == 2) {
        strcpy(pass, "Jordinian");
    } else {
        strcpy(pass, "Foreign");
    }

    printf(" [%s Passenger_%d ======> Border_%d]\n", pass, getpid(),
           queuesIds[chosenBorder]);
    fflush(stdout);
    passengerDetails details;
    details.passengerPid = getpid();
    details.validPassport = passportExpired;
    state = 1;

    if (msgsnd(queuesIds[chosenBorder], &details, sizeof(details), 0) == -1) {
        perror("Error Sending Passenger's Details");
        exit(3);
    }

    while (1) {
        patienceLevel--;
        sleep(0.5);
    }

    return 0;
}

void goToTheBus(int n) {
    state = 3;
}

void checkPatienceLevel(sharedMemory *hall) {
    if (patienceLevel <= 0) {
        hall->Pu = hall->Pu + 1;
        printf(
            " [Passenger_%d got impatient, total impatient passengers: %d]\n",
            getpid(), hall->Pu);
        fflush(stdout);
        if (hall->Pu >= turnedBack) {
            kill(getppid(), SIGUSR1);
        }
        if (semop(semId, &release, 1) == -1) {
            perror("semop -- producer -- release");
            exit(4);
        }
        exit(0);
    }
}

void moveOnHandler(int n) {
    acquire.sem_num = 0;
    if (semop(semId, &acquire, 1) == -1) {
        perror("semop -- producer -- acquire");
        exit(4);
    }

    sharedMemory *hall = (sharedMemory *)shHallPtr;
    checkPatienceLevel(hall);
    addToHall(hall, getpid());
    hall->Pg = hall->Pg + 1;
    printf(" [Passenger_%d ======> Hall, total gratned access: %d]\n", getpid(),
           hall->Pg);
    fflush(stdout);
    if (hall->Pg >= accessGranted) {
        kill(getppid(), SIGUSR1);
    }
    state = 2;
    release.sem_num = 0;

    if (semop(semId, &release, 1) == -1) {
        perror("semop -- producer -- release");
        exit(5);
    }
}

void passportExpiredHandler(int n) {
    acquire.sem_num = 0;
    if (semop(semId, &acquire, 1) == -1) {
        perror("semop -- producer -- acquire");
        exit(4);
    }

    sharedMemory *hall = (sharedMemory *)shHallPtr;
    checkPatienceLevel(hall);
    hall->Pd = hall->Pd + 1;
    printf(" [Passenger_%d has expired passport, total denied access: %d]\n",
           getpid(), hall->Pd);
    fflush(stdout);
    if (hall->Pd >= accessDenied) {
        kill(getppid(), SIGUSR1);
    }
    release.sem_num = 0;

    if (semop(semId, &release, 1) == -1) {
        perror("semop -- producer -- release");
        exit(5);
    }
    exit(0);
}