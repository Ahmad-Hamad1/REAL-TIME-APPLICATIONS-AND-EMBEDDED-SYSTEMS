#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("Missing arguments");
        exit(-8);
    }
    int readDescriptor, writeDescriptor;
    readDescriptor = atoi(argv[1]);
    writeDescriptor = atoi(argv[2]);
    char file1Name[20], file2Name[20];
    while (1) {
        char message[30];
        // waiting for file names from the parent
        if (read(readDescriptor, &message, sizeof(message)) == -1) {
            perror("Reading failed");
            exit(-5);
        }
        int arr1[11], arr2[11];
        char *names;
        names = strtok(message, "-");
        strcpy(file1Name, names);
        names = strtok(NULL, "-");
        strcpy(file2Name, names);
        // reading the first file
        FILE *filePtr1 = fopen(file1Name, "r");
        for (int i = 0; i < 10; i++) {
            fscanf(filePtr1, "%d", &arr1[i]);
        }
        fclose(filePtr1);
        // reading the second file
        FILE *filePtr2 = fopen(file2Name, "r");
        for (int i = 0; i < 10; i++) {
            fscanf(filePtr2, "%d", &arr2[i]);
        }
        fclose(filePtr2);
        // printing the choices of the first player to the user
        printf(" Player 1 chooses: [ ");
        fflush(stdout);
        for (int i = 0; i < 10; i++) {
            if (arr1[i] == 100) {
                printf("%d ", arr1[i]);
            } else if (arr1[i] > 9) {
                printf(" %d ", arr1[i]);
            } else if (i != 9) {
                printf(" %d  ", arr1[i]);
            } else {
                printf(" %d ", arr1[i]);
            }
        }
        // printing the choices of the second player to the user
        printf("]\n\n Player 2 chooses: [ ");
        fflush(stdout);
        for (int i = 0; i < 10; i++) {
            if (arr2[i] == 100) {
                printf("%d ", arr2[i]);
            } else if (arr2[i] > 9) {
                printf(" %d ", arr2[i]);
            } else if (i != 9) {
                printf(" %d  ", arr2[i]);
            } else {
                printf(" %d ", arr2[i]);
            }
        }
        printf("]\n\n");
        fflush(stdout);
        int score1 = 0, score2 = 0;
        // calculating round score for the two players
        for (int i = 0; i < 10; i++) {
            if (arr1[i] > arr2[i])
                score1++;
            else if (arr2[i] > arr1[i])
                score2++;
        }
        char result[10];
        sprintf(result, "%d-%d", score1, score2);
        // removing the two files
        remove(file1Name);
        remove(file2Name);
        // writing the scores back to the parent
        if (write(writeDescriptor, result, sizeof(result)) == -1) {
            perror("Writing failed");
            exit(-7);
        }
    }
    remove(file1Name);
    remove(file2Name);
    close(readDescriptor);
    close(writeDescriptor);
    return 0;
}
