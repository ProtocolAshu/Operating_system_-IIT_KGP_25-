#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_SIZE 2004 // 100 globals + 5*200 waiter queues + 600+4 cook queue
#define MINUTE 100000 // 100ms per minute
#define COOK_TIME_PER_PERSON 5 // 5 minutes per person

// Semaphore operations
struct sembuf wait_sem = {0, -1, 0};
struct sembuf signal_sem = {0, 1, 0};

int *M; // Shared memory
int shmid, sem_mutex, sem_cook, sem_waiter[5], sem_customer;

void init_ipc() {
    key_t key = ftok("cook.c", 'R');
    shmid = shmget(key, SHM_SIZE * sizeof(int), IPC_CREAT | 0666);
    M = (int *)shmat(shmid, NULL, 0);
    M[0] = 0;        // time
    M[1] = 10;       // empty tables
    M[2] = 0;        // next waiter
    M[3] = 0;        // cook queue size
    M[1100] = 1100;  // cook queue front
    M[1101] = 1100;  // cook queue back

    sem_mutex = semget(key, 1, IPC_CREAT | 0666);
    semctl(sem_mutex, 0, SETVAL, 1);

    sem_cook = semget(key + 1, 1, IPC_CREAT | 0666);
    semctl(sem_cook, 0, SETVAL, 0);

    for (int i = 0; i < 5; i++) {
        sem_waiter[i] = semget(key + 2 + i, 1, IPC_CREAT | 0666);
        semctl(sem_waiter[i], 0, SETVAL, 0);
        M[100 + i * 200 + 198] = 0; // PO for waiter i
        M[100 + i * 200 + 199] = 0; // FR for waiter i
    }

    sem_customer = semget(key + 7, 200, IPC_CREAT | 0666);
    for (int i = 0; i < 200; i++)
        semctl(sem_customer, i, SETVAL, 0);
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

void cmain(int id) {
    char name = 'C' + id;
    printf("[11:00 am] Cook %c is ready\n", name);
    while (1) {
        semop(sem_cook, &wait_sem, 1); // Wait for cooking request
        semop(sem_mutex, &wait_sem, 1);
        if (M[0] >= 240 && M[3] == 0) { // 3:00pm and no orders
            if (id == 1) { // Last cook wakes waiters
                for (int i = 0; i < 5; i++)
                    semop(sem_waiter[i], &signal_sem, 1);
            }
            semop(sem_mutex, &signal_sem, 1);
            break;
        }
        int front = M[1100];
        int waiter_id = M[front], cust_id = M[front + 1], count = M[front + 2];
        M[1100] = front + 3;
        M[3]--;
        semop(sem_mutex, &signal_sem, 1);

        printf("[%02d:%02d %s] Cook %c: Preparing order (Waiter %c, Customer %d, Count %d)\n",
               M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", name, 'U' + waiter_id, cust_id, count);

        int curr_time = M[0];
        usleep(COOK_TIME_PER_PERSON * count * MINUTE);
        update_time(curr_time, COOK_TIME_PER_PERSON * count);

        printf("[%02d:%02d %s] Cook %c: Prepared order (Waiter %c, Customer %d, Count %d)\n",
               M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", name, 'U' + waiter_id, cust_id, count);

        semop(sem_mutex, &wait_sem, 1);
        M[100 + waiter_id * 200 + 199] = cust_id; // Set FR
        semop(sem_mutex, &signal_sem, 1);
        semop(sem_waiter[waiter_id], &signal_sem, 1); // Notify waiter
    }
    printf("[%02d:%02d %s] Cook %c: Leaving\n", M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", name);
    exit(0);
}

int main() {
    init_ipc();
    for (int i = 0; i < 2; i++) {
        if (fork() == 0) {
            cmain(i);
        }
    }
    wait(NULL);
    wait(NULL);
    return 0;
}