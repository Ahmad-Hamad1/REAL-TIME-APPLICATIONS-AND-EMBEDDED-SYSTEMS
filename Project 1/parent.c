#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int bigScore1, bigScore2;

void player1Ready(int signal);

void player2Ready(int signal);

void gameEnds(int signal);

int player1Finished = 0;
int player2Finished = 0;
int playersIds[2];
int rounds = 1;
int done = 0;
int refereeId;

int main() {
    // forking the two players
    for (int i = 0; i < 2; i++) {
        int id = fork();
        if (id == -1) {
            perror("Fork failed");
            exit(-i - 1);
        }
        if (id == 0) {
            char playerNum[5];
            sprintf(playerNum, "%d", i + 1);
            execlp("./child", "child", playerNum, 0);
            perror("Child exec failed");
            exit(-7);
        }
        playersIds[i] = id;
        sleep(1);
    }
    // declaring two file descriptors to communicate between the parent and the referee
    int fd1[2], fd2[2];
    // initializing the two pipes
    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        perror("Pipe creation failed");
        exit(-4);
    }
    //forking the referee
    int id = fork();
    if (id == -1) {
        perror("Fork failed");
        exit(-3);
    }
    if (id == 0) { // referee
        close(fd1[1]);
        close(fd2[0]);
        char readDescriptor[20], writeDescriptor[20];
        sprintf(readDescriptor, "%d", fd1[0]);
        sprintf(writeDescriptor, "%d", fd2[1]);
        execlp("./referee", "referee", readDescriptor, writeDescriptor);
        perror("Referee exec failed");
        exit(-9);
    }
    // parent section
    refereeId = id;
    sleep(1);
    close(fd1[0]);
    close(fd2[1]);
    signal(SIGINT, &player1Ready);
    signal(SIGQUIT, &player2Ready);
    printf("\n");
    fflush(stdout);
    while (1) {
        printf(" ========================== Round %d =========================== \n\n", rounds);
        fflush(stdout);
        // informing the first player to write his numbers
        kill(playersIds[0], SIGUSR1);
        // waiting for the first player until the numbers were written
        while (player1Finished == 0);
        // informing the second player to write his numbers
        kill(playersIds[1], SIGUSR1);
        // waiting for the second player until the numbers were written
        while (player2Finished == 0);
        // writing files names to the referee
        if (write(fd1[1], "child1.txt-child2.txt", sizeof("child1.txt-child2.txt")) == -1) {
            perror("Writing failed");
            exit(-7);
        }
        char result[10];
        // reading round scores from the referee
        if (read(fd2[0], result, sizeof(result)) == -1) {
            perror("Reading failed");
            exit(-6);
        }
        // splitting the two scores and incrementing the global scores using them
        int score1, score2;
        char *score;
        score = strtok(result, "-");
        score1 = atoi(score);
        score = strtok(NULL, "-");
        score2 = atoi(score);
        printf("\n");
        fflush(stdout);
        printf(" Current round scores:\n Player1: %d \n Player2: %d\n\n", score1, score2);
        fflush(stdout);
        bigScore1 += score1;
        bigScore2 += score2;
        printf(" The overall scores:\n Player1: %d\n Player2: %d\n\n", bigScore1, bigScore2);
        fflush(stdout);
        // checking if the two players exceed the score of 50
        if (bigScore1 >= 50 && bigScore2 >= 50) {
            printf(" Game ends with %d rounds and the winners of the whole game are\n player1 and player 2\n", rounds);
            fflush(stdout);
            kill(playersIds[0], SIGKILL);
            kill(playersIds[1], SIGKILL);
            close(fd1[1]);
            close(fd2[0]);
            kill(refereeId, SIGKILL);
            exit(0);
        } else if (bigScore1 >= 50 || bigScore2 >= 50) { // checking if any player exceeds a score of 50
            printf(" Game ends with %d rounds and the winner of the whole game is player%d\n\n", rounds,
                   bigScore1 >= 50 ? 1 : 2);
            fflush(stdout);
            kill(playersIds[0], SIGKILL);
            kill(playersIds[1], SIGKILL);
            close(fd1[1]);
            close(fd2[0]);
            kill(refereeId, SIGKILL);
            exit(0);
        }
        player1Finished = player2Finished = 0;
        rounds++;
        sleep(4);
    }
    return 0;
}

// signal handler for the signal coming from the first child
void player1Ready(int signal) {
    player1Finished = 1;
}

// signal handler for the signal coming from the second child
void player2Ready(int signal) {
    player2Finished = 1;
}

