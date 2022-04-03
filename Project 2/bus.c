#include "local.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("Insufficient Arguments");
        exit(1);
    }

    readInputsFile();

    int shmID = atoi(argv[1]);
    int hallSemID = atoi(argv[2]);
    int busSemID = atoi(argv[3]);
    char *shmPtr;
    sharedMemory *memPtr;

    if ((shmPtr = (char *)shmat(shmID, 0, 0)) == (char *)-1) {
        exit(-2);
    }

    memPtr = (sharedMemory *)shmPtr;

    int numberOfPassengers = 0;
    srand(getpid() | getppid());
    int passengerIDs[busCapacity];

    while (1) {
        acquireBus.sem_num = 0;
        if (semop(busSemID, &acquireBus, 1) == -1) {
            perror("semop -- producer -- acquire");
            exit(2);
        }

        /* the bus should take passengers as long as there are passengers in
         * hall as well as the bus is not full. */
        int passengerID = -1;
        while (numberOfPassengers < busCapacity) {
            acquire.sem_num = 0;
            if (semop(hallSemID, &acquire, 1) == -1) {
                perror("semop -- producer -- acquire");
                exit(3);
            }

            passengerID = removeFromHall(memPtr);
            /* change the state of passenger to be bus passenger. */
            if (passengerID != -1) {
                passengerIDs[numberOfPassengers++] = passengerID;
                printf("\n [Passenger_%d ======> Bus_%d]\n\n", passengerID, getpid());
                fflush(stdout);
            }

            release.sem_num = 0;
            if (semop(hallSemID, &release, 1) == -1) {
                perror("semop -- producer -- release");
                exit(4);
            }

            sleep(1);
        }
        printf("\n [Bus_%d has left to the other side] \n\n", getpid());
        fflush(stdout);

        releaseBus.sem_num = 0;
        if (semop(busSemID, &releaseBus, 1) == -1) {
            perror("semop -- producer -- release");
            exit(5);
        }

        sleep((rand() % (maxBusRange - minBusRange + 1)) + minBusRange);
        for (int i = 0; i < busCapacity; i++) {
            kill(passengerIDs[i], SIGKILL);
        }
        numberOfPassengers = 0;
    }

    return 0;
}