#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SHM_SIZE 2004
#define MINUTE 100000
#define EAT_TIME 30

struct sembuf wait_sem = {0, -1, 0};
struct sembuf signal_sem = {0, 1, 0};

int *M;
int shmid, sem_mutex, sem_cook, sem_waiter[5], sem_customer;

void init_ipc() {
    key_t key = ftok("cook.c", 'R');
    shmid = shmget(key, SHM_SIZE * sizeof(int), 0666);
    M = (int *)shmat(shmid, NULL, 0);

    sem_mutex = semget(key, 1, 0666);
    sem_cook = semget(key + 1, 1, 0666);
    for (int i = 0; i < 5; i++)
        sem_waiter[i] = semget(key + 2 + i, 1, 0666);
    sem_customer = semget(key + 7, 200, 0666);
}

void update_time(int curr_time, int delay) {
    int new_time = curr_time + delay;
    semop(sem_mutex, &wait_sem, 1);
    if (new_time < M[0]) {
        printf("[Warning] Attempt to set time to %d fails (current time = %d)\n", new_time, M[0]);
    } else {
        M[0] = new_time;
    }
    semop(sem_mutex, &signal_sem, 1);
}

void cmain(int id, int arrival, int count) {
    semop(sem_mutex, &wait_sem, 1);
    if (arrival >= 240) { // After 3:00pm
        printf("[%02d:%02d %s]\t\t\t\tCustomer %d leaves (late arrival)\n",
               arrival / 60 + 11, arrival % 60, arrival < 120 ? "am" : "pm", id);
        semop(sem_mutex, &signal_sem, 1);
        exit(0);
    }
    if (M[1] == 0) { // No empty tables
        printf("[%02d:%02d %s]\t\t\t\tCustomer %d leaves (no empty table)\n",
               arrival / 60 + 11, arrival % 60, arrival < 120 ? "am" : "pm", id);
        semop(sem_mutex, &signal_sem, 1);
        exit(0);
    }
    M[1]--; // Occupy a table
    int waiter_id = M[2];
    M[2] = (M[2] + 1) % 5;
    if (M[0] < arrival) M[0] = arrival;
    semop(sem_mutex, &signal_sem, 1);

    printf("[%02d:%02d %s] Customer %d arrives (count = %d)\n",
           M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", id, count);

    int base = 100 + waiter_id * 200;
    semop(sem_mutex, &wait_sem, 1);
    int back = M[base + 1];
    M[back] = id; M[back + 1] = count;
    M[base + 1] = back + 2;
    M[base + 198]++; // Increment PO
    semop(sem_mutex, &signal_sem, 1);
    printf("[%02d:%02d %s]\tCustomer %d: Order placed to Waiter %c\n",
           M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", id, 'U' + waiter_id);
    semop(sem_waiter[waiter_id], &signal_sem, 1);

    struct sembuf customer_wait = {id - 1, -1, 0}; // Wait on customer semaphore id-1
    semop(sem_customer, &customer_wait, 1); // Corrected semop call
    printf("[%02d:%02d %s]\t\tCustomer %d gets food [Waiting time = %d]\n",
           M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", id, M[0] - arrival);

    int curr_time = M[0];
    usleep(EAT_TIME * MINUTE);
    update_time(curr_time, EAT_TIME);

    semop(sem_mutex, &wait_sem, 1);
    M[1]++; // Free the table
    semop(sem_mutex, &signal_sem, 1);
    printf("[%02d:%02d %s]\t\tCustomer %d finishes eating and leaves\n",
           M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", id);
    exit(0);
}

int main() {
    init_ipc();
    FILE *fp = fopen("customers.txt", "r");
    int id, arrival, count, prev_arrival = 0;
    while (fscanf(fp, "%d", &id) == 1 && id != -1) {
        fscanf(fp, "%d %d", &arrival, &count);
        if (arrival > prev_arrival) {
            usleep((arrival - prev_arrival) * MINUTE);
            prev_arrival = arrival;
        }
        if (fork() == 0) {
            cmain(id, arrival, count);
        }
    }
    fclose(fp);
    while (wait(NULL) > 0); // Wait for all children
    shmctl(shmid, IPC_RMID, NULL);
    semctl(sem_mutex, 0, IPC_RMID);
    semctl(sem_cook, 0, IPC_RMID);
    for (int i = 0; i < 5; i++)
        semctl(sem_waiter[i], 0, IPC_RMID);
    semctl(sem_customer, 0, IPC_RMID);
    return 0;
}