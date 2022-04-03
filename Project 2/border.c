#include "local.h"

void enqueue(int passengerPid, int validPassport);
node *dequeue();

int time = 0;
int giveAccess = 1;
node *front = NULL;
node *rear = NULL;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        perror("missing arguements\n");
        exit(-5);
    }

    readInputsFile();

    srand((unsigned int)getpid());
    int shHallId = atoi(argv[1]);
    int semId = atoi(argv[2]);
    int msgId = atoi(argv[3]);
    int rate = (rand() % 4) + 1;
    int hallFull = 0;
    double sleepPeriod = 1.0 / rate;
    char *shHallPtr;

    if ((shHallPtr = (char *)shmat(shHallId, 0, 0)) == (char *)-1) {
        perror("Error Getting Shared Memory");
        exit(2);
    }
    sharedMemory *hall = (sharedMemory *)shHallPtr;

    while (1) {
        int currentRead = (rand() % 5) + 1;
        int num_messages;
        struct msqid_ds buf;
        passengerDetails passenger;

        if (msgctl(msgId, IPC_STAT, &buf) == -1) {
            perror("msg-ctl: ");
            exit(5);
        }

        num_messages = buf.msg_qnum;

        if (num_messages < currentRead) currentRead = num_messages;

        for (int i = 0; i < currentRead; i++) {
            if (msgrcv(msgId, &passenger, sizeof(passenger), 0, 0) == -1)
                exit(-6);
            enqueue(passenger.passengerPid, passenger.validPassport);
        }

        acquire.sem_num = 0;
        if (semop(semId, &acquire, 1) == -1) {
            perror("semop -- producer -- acquire");
            exit(4);
        }

        hallFull = isHallFull(hall);
        release.sem_num = 0;
        if (checkThreshold(hall)) {
            giveAccess = 1;
        }

        if (semop(semId, &release, 1) == -1) {
            perror("semop -- producer -- release");
            exit(5);
        }

        if (!hallFull && giveAccess) {
            node *front = dequeue();
            if (front) {
                if (front->validPassport > 90) {
                    kill(front->passengerPid, SIGUSR2);
                } else {
                    kill(front->passengerPid, SIGUSR1);
                }
            }
        } else {
            giveAccess = 0;
        }
        sleep(sleepPeriod);
    }

    return 0;
}

void enqueue(int passengerPid, int validPassport) {
    node *newNode = (struct node *)malloc(sizeof(node));
    newNode->passengerPid = passengerPid;
    newNode->validPassport = validPassport;
    newNode->next = NULL;

    // if it is the first node
    if (front == NULL && rear == NULL) {
        // make both front and rear points to the new node
        front = rear = newNode;
        return;
    }

    else {
        // add new node in rear->next
        rear->next = newNode;
        // make the new node as the rear node
        rear = newNode;
    }
}

node *dequeue() {
    // used to free the first node after dequeue
    node *temp;
    if (front == NULL) return NULL;

    // take backup
    temp = front;

    // make the front node points to the next node
    // logically removing the front element
    front = front->next;

    // if front == NULL, set rear = NULL
    if (front == NULL) rear = NULL;

    // free the first node
    return temp;
}