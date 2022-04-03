#include "local.h"

int main(int argc, char const *argv[]) {
    // read needed variables from inputs.txt file.
    readInputsFile();

    // clear the array of lists.
    for (int i = 0; i < 10; i++) {
        linesLists[i] = NULL;
    }

    // clear the order step array.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 4; j++) {
            orderedSteps[i][j] = 0;
        }
    }

    // initiate the mutexes
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 5; j++) {
            if (pthread_mutex_init(&locks[i][j], NULL)) {
                perror("Initiate mutex");
                exit(1);
            }
        }
    }

    for (int i = 0; i < 10; i++) {
        if (pthread_mutex_init(&cartonLocks[i], NULL)) {
            perror("Initiate mutex");
            exit(1);
        }
    }

    // create the line employee threads.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            ThreadParam *threadParam = malloc(sizeof(threadParam));
            if (threadParam == NULL) {
                perror("error in malloc");
                exit(1);
            }
            threadParam->lineNumber = i;
            threadParam->empNumber = j;
            if (pthread_create(&employeesIds[i][j], NULL, (void *)linesEmployee,
                               (void *)(threadParam))) {
                perror("Create thread");
                exit(1);
            }
        }
    }

    // create the storage employee threads.
    for (int i = 0; i < 10; i++) {
        int j = i;
        if (pthread_create(&storageEmps[i], NULL, (void *)storageEmpWork,
                           (void *)&j)) {
            perror("Create thread");
            exit(1);
        }
    }

    // create the loading employee threads.
    for (int i = 0; i < NUM_OF_LOADING_EMPLOYEES; i++) {
        if (pthread_create(&loadingEmployees[i], NULL, (void *)loadingEmpWork,
                           (void *)&ids[i])) {
            perror("Create thread");
            exit(1);
        }
    }

    // create all of CEO, HR and accountant threads.
    if (pthread_create(&CEOThread, NULL, (void *)CEO, NULL) ||
        pthread_create(&HRThread, NULL, (void *)HR, NULL) ||
        pthread_create(&accountantThread, NULL, (void *)accountant, NULL)) {
        perror("Create thread");
        exit(1);
    }

    // join all threads.
    for (int i = 0; i < 10; i++) {
        if (pthread_join(storageEmps[i], NULL)) {
            perror("Join thread");
            exit(1);
        }
    }

    // join all threads.
    if (pthread_join(CEOThread, NULL) || pthread_join(HRThread, NULL) ||
        pthread_join(accountantThread, NULL)) {
        perror("Join thread");
        exit(1);
    }

    // join loading employees.
    for (int i = 0; i < NUM_OF_LOADING_EMPLOYEES; i++) {
        if (pthread_join(loadingEmployees[i], NULL)) {
            perror("Join thread");
            exit(1);
        }
    }

    // join lines employee threads.
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (pthread_join(employeesIds[i][j], NULL)) {
                perror("Join thread");
                exit(1);
            }
        }
    }
    return 0;
}

Laptop *copyLaptop(Laptop *laptop) {
    Laptop *lap = (Laptop *)malloc(sizeof(Laptop));

    if (lap == NULL) {
        perror("error in malloc");
        exit(1);
    }

    lap->id = laptop->id;
    lap->finishedSteps = laptop->finishedSteps;
    for (int i = 0; i < 10; i++) {
        lap->steps[i] = laptop->steps[i];
    }
    return lap;
}

void addToList(Laptop *lap, int lineNum) {
    Node *front = linesLists[lineNum];
    if (front == NULL) {
        front = (Node *)malloc(sizeof(Node));

        if (front == NULL) {
            perror("error in malloc");
            exit(1);
        }

        front->laptop = copyLaptop(lap);
        front->next = NULL;
        linesLists[lineNum] = front;
        return;
    }

    Node *temp;
    temp = (Node *)malloc(sizeof(Node));

    if (temp == NULL) {
        perror("error in malloc");
        exit(1);
    }

    temp->laptop = copyLaptop(lap);
    temp->next = front;
    linesLists[lineNum] = temp;
}

Laptop *getFromList(int empNumber, int lineNum) {
    Node *front = linesLists[lineNum];

    if (front == NULL) {  // empty list.
        return NULL;
    } else if (front->next == NULL) {  // list with size = 1
        if (front->laptop->steps[empNumber] == 0) {
            Laptop *lap = copyLaptop(front->laptop);
            free(front->laptop);
            free(front);
            linesLists[lineNum] = NULL;
            return lap;
        }
    }

    Node *prev = front;
    Node *temp = prev->next;
    // iterate till it finds a laptop which its
    // steps[empNumber] is not ready yet.
    while (temp != NULL) {
        if (temp->laptop->steps[empNumber] == 0) {
            prev->next = temp->next;
            temp->next = NULL;
            Laptop *lap = copyLaptop(temp->laptop);
            free(temp->laptop);
            free(temp);
            return lap;
        }
        prev = prev->next;
        temp = temp->next;
    }
    return NULL;
}

void *linesEmployee(void *arg) {
    // line employee from 1-5 will work in order.
    // line employee from 6-10 will work randomly.

    ThreadParam param = *((ThreadParam *)arg);
    int lineNumber = param.lineNumber;
    int empNumber = param.empNumber;
    int prevEmpNumber = empNumber - 1;
    int laptopsNum = 0;

    while (true) {
        usleep(50000);
        if (pthread_mutex_lock(&empsSuspend)) {
            perror("Error in mutex");
            exit(1);
        }
        if (suspendedLines[lineNumber] == 1) {
            if (pthread_cond_wait(&empsSuspendCondition, &empsSuspend)) {
                perror("Error in cond_var");
                exit(1);
            }
        }

        if (pthread_mutex_unlock(&empsSuspend)) {
            perror("Error in mutex");
            exit(1);
        }

        if (empNumber == 0) {
            if (pthread_mutex_lock(&locks[lineNumber][0])) {
                perror("Error in mutex");
                exit(1);
            }
            fflush(stdout);
            orderedSteps[lineNumber][empNumber]++;
            if (pthread_mutex_unlock(&locks[lineNumber][0])) {
                perror("Error in mutex");
                exit(1);
            }

        } else if (empNumber != 0 && empNumber < 4) {
            if (pthread_mutex_lock(&locks[lineNumber][prevEmpNumber])) {
                perror("Error in mutex");
                exit(1);
            }

            if (orderedSteps[lineNumber][prevEmpNumber] != 0) {
                if (pthread_mutex_lock(&locks[lineNumber][empNumber])) {
                    perror("Error");
                    exit(1);
                }
                orderedSteps[lineNumber][prevEmpNumber]--;
                orderedSteps[lineNumber][empNumber]++;
                if (pthread_mutex_unlock(&locks[lineNumber][empNumber])) {
                    perror("Error");
                    exit(1);
                }
            }

            if (pthread_mutex_unlock(&locks[lineNumber][prevEmpNumber])) {
                perror("Error in mutex");
                exit(1);
            }

        } else if (empNumber == 4) {
            if (pthread_mutex_lock(&locks[lineNumber][3])) {
                perror("Error");
                exit(1);
            }
            if (orderedSteps[lineNumber][3] != 0) {
                if (pthread_mutex_lock(&locks[lineNumber][4])) {
                    perror("Error");
                    exit(1);
                }

                Laptop *lap = malloc(sizeof(Laptop));
                lap->id = ++laptopIDS;
                lap->finishedSteps = 5;
                for (int i = 0; i < 10; i++) lap->steps[i] = 0;
                addToList(lap, lineNumber);
                orderedSteps[lineNumber][3]--;

                if (pthread_mutex_unlock(&locks[lineNumber][4])) {
                    perror("Error");
                    exit(1);
                }
            }
            if (pthread_mutex_unlock(&locks[lineNumber][3])) {
                perror("Error");
                exit(1);
            }

        } else if (empNumber >= 4 && empNumber < 10) {  // 5-9
            if (pthread_mutex_lock(&locks[lineNumber][4])) {
                perror("Error");
                exit(1);
            }
            Laptop *lap = getFromList(empNumber, lineNumber);

            if (pthread_mutex_unlock(&locks[lineNumber][4])) {
                perror("Error");
                exit(1);
            }

            if (lap != NULL) {
                lap->steps[empNumber] = 1;
                lap->finishedSteps++;
                if (lap->finishedSteps == 10) {
                    if (pthread_mutex_lock(&laptopLock)) {
                        perror("Error");
                        exit(1);
                    }
                    laptopsNum++;
                    if (laptopsNum == 10) {
                        if (pthread_mutex_lock(&cartonLocks[lineNumber])) {
                            perror("Error");
                            exit(1);
                        }
                        cartonNum[lineNumber]++;
                        laptopsNum = 0;
                        if (pthread_mutex_unlock(&cartonLocks[lineNumber])) {
                            perror("Error");
                            exit(1);
                        }
                    }
                    if (pthread_mutex_unlock(&laptopLock)) {
                        perror("Error");
                        exit(1);
                    }

                } else {
                    if (pthread_mutex_lock(&locks[lineNumber][4])) {
                        perror("Error");
                        exit(1);
                    }
                    addToList(lap, lineNumber);
                    if (pthread_mutex_unlock(&locks[lineNumber][4])) {
                        perror("Error");
                        exit(1);
                    }
                }
            }
        }
    }
}

void *storageEmpWork(void *arg) {
    // each storage employee will be responsible for one carton of laptops.
    int empId = *(int *)arg;
    while (1) {
        if (pthread_mutex_lock(&addToStorage)) {
            perror("Error");
            exit(1);
        }
        if (isAddToStorage && storageCount >= maxStorageCapacity) {
            isAddToStorage = 0;
            printf("Storage room is full !!\n\n");
            fflush(stdout);
        } else if (!isAddToStorage && storageCount < minStorageCapacity) {
            isAddToStorage = 1;
            printf("Storage goes down again !!\n\n");
            fflush(stdout);
        }
        if (isAddToStorage == 1) {
            if (pthread_mutex_lock(&cartonLocks[empId])) {
                perror("Error");
                exit(1);
            }
            if (cartonNum[empId] > 0) {
                if (pthread_mutex_lock(&storageLock)) {
                    perror("Error");
                    exit(1);
                }
                cartonNum[empId]--;
                storageCount++;
                printf("New laptop is added to the storage room from line %d\n",
                       empId);
                fflush(stdout);
                printf("Total number of laptops in storage room ==> %d\n\n",
                       storageCount);
                fflush(stdout);
                if (pthread_mutex_unlock(&storageLock)) {
                    perror("Error");
                    exit(1);
                }
            }
            if (pthread_mutex_unlock(&cartonLocks[empId])) {
                perror("Error");
                exit(1);
            }
        }
        if (pthread_mutex_unlock(&addToStorage)) {
            perror("Error");
            exit(1);
        }
        sleep((rand() % (storageEmpMaxDelay - storageEmpMinDelay)) + 1);
    }
}

void *loadingEmpWork(void *arg) {
    // each loading employee will be responsible for one truck to fill it.
    int truckNumber = *(int *)arg;
    int truckCartons = 0;
    while (1) {
        if (pthread_mutex_lock(&storageLock)) {
            perror("Error");
            exit(1);
        }
        if (storageCount > 0) {
            storageCount--;
            truckCartons++;
            printf("New added carton to truck %d\n", truckNumber);
            fflush(stdout);
            printf("Truck %d has %d cartons\n\n", truckNumber, truckCartons);
            fflush(stdout);
            if (truckCartons == truckCapacity) {
                printf("Truck %d is full and has left\n\n", truckNumber);
                fflush(stdout);
                truckCartons = 0;
                if (pthread_mutex_lock(&soldLaptopsLock)) {
                    perror("Error");
                    exit(1);
                }
                soldLaptops += truckCapacity * 10;
                if (pthread_mutex_unlock(&soldLaptopsLock)) {
                    perror("Error");
                    exit(1);
                }

                if (pthread_mutex_unlock(&storageLock)) {
                    perror("Error");
                    exit(1);
                }
                sleep((rand() % (truckMaxDelay - truckMinDelay + 1)) +
                      truckMinDelay);
                continue;
            }
        }
        if (pthread_mutex_unlock(&storageLock)) {
            perror("Error");
            exit(1);
        }
        usleep(50000);
    }
}

void *HR(void *args) {
    // checks the profit and signals the CEO in three different cases as
    // following: 1) in case the profit >= max threshold of profit. 2) in case
    // the profit < min threshold of profit. 3) in case the profit still
    // decreasing after suspension another line.
    int tellCeoToSuspendEmps = 0;
    while (1) {
        if (pthread_mutex_lock(&profitLock)) {
            perror("Error");
            exit(1);
        }
        if (profit < profitThreshold) {
            tellCeoToSuspendEmps = 1;
            if (pthread_mutex_lock(&suspend)) {
                perror("Error");
                exit(1);
            }
            suspendLines = 1;
            printf("Profit goes under the threshold. Stop some lines !!\n");
            fflush(stdout);
            if (pthread_cond_broadcast(&suspensionOfLinesCondition)) {
                perror("Error");
                exit(1);
            }
            if (pthread_mutex_unlock(&suspend)) {
                perror("Error");
                exit(1);
            }
        } else if (tellCeoToSuspendEmps && profit >= profitThreshold) {
            tellCeoToSuspendEmps = 0;
            if (pthread_mutex_lock(&suspend)) {
                perror("Error");
                exit(1);
            }
            suspendLines = 0;
            printf("Profit goes over the threshold. Activate some lines !!\n");
            fflush(stdout);
            if (pthread_cond_broadcast(&suspensionOfLinesCondition)) {
                perror("Error");
                exit(1);
            }
            if (pthread_mutex_unlock(&suspend)) {
                perror("Error");
                exit(1);
            }
        }
        if (pthread_mutex_unlock(&profitLock)) {
            perror("Error");
            exit(1);
        }
        sleep(4);
    }
}

void *CEO(void *arg) {
    // The CEO either suspends a new line or unsuspends a stop-work line after
    // getting a signal from the HR that tells the CEO the status of the profit.
    if (pthread_mutex_lock(&suspend)) {
        perror("Error");
        exit(1);
    }
    while (1) {
        if (pthread_cond_wait(&suspensionOfLinesCondition, &suspend)) {
            perror("Error");
            exit(1);
        }
        if (suspendLines == 1) {
            linesToSuspend++;
            printf("%d line(s) were suspended !!\n\n", linesToSuspend);
            fflush(stdout);
            if (linesToSuspend > 5) {
                exitProgram();
            }
        }
        if (pthread_mutex_lock(&empsSuspend)) {
            perror("Error");
            exit(1);
        }
        for (int i = 0; i < linesToSuspend; i++) {
            suspendedLines[i] = suspendLines;
        }
        if (suspendLines == 0) {
            printf("%d line(s) were unsuspended !!\n\n", linesToSuspend);
            fflush(stdout);
            linesToSuspend = 0;
            if (pthread_cond_broadcast(&empsSuspendCondition)) {
                perror("Error");
                exit(1);
            }
        }
        if (pthread_mutex_unlock(&empsSuspend)) {
            perror("Error");
            exit(1);
        }
    }
}

void *accountant(void *arg) {
    // calculate total profits after value of sleep to check if the profits
    // overflows the threshold.
    int expenses = hrSalary + ceoSalary + extraEmpsSalary + storageEmpSalary +
                   ((loadingEmpSalary + truckDriversSalary) * 10);
    int currentExpense = 0;
    int laptopsProfit;
    while (1) {
        if (pthread_mutex_lock(&soldLaptopsLock)) {
            perror("Error");
            exit(1);
        }
        if (pthread_mutex_lock(&empsSuspend)) {
            perror("Error");
            exit(1);
        }
        currentExpense += expenses;
        for (int i = 0; i < 10; i++) {
            if (suspendedLines[i] == 0) currentExpense += 10 * techEmpSalary;
        }
        if (pthread_mutex_unlock(&empsSuspend)) {
            perror("Error");
            exit(1);
        }
        laptopsProfit = soldLaptops * (laptopPrice - laptopCost);
        if (pthread_mutex_unlock(&soldLaptopsLock)) {
            perror("Error");
            exit(1);
        }
        if (pthread_mutex_lock(&profitLock)) {
            perror("Error");
            exit(1);
        }
        profit = (laptopsProfit - currentExpense);
        printf("Current profit is : %d\n\n", profit);
        fflush(stdout);
        if (profit >= profitToEnd) {
            printf("Profit ==> %d\n\n", profit);
            fflush(stdout);
            exitProgram();
        }
        if (pthread_mutex_unlock(&profitLock)) {
            perror("Error");
            exit(1);
        }
        sleep(4);
    }
}