#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

#define SHM_SIZE 2004
#define MINUTE 100000

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

void wmain(int id) {
    char name = 'U' + id;
    printf("[11:00 am] %sWaiter %c is ready\n", "\t" + (id ? 1 : 0) * (id - 1), name);
    int base = 100 + id * 200;
    M[base] = base + 2; // front
    M[base + 1] = base + 2; // back
    while (1) {
        semop(sem_waiter[id], &wait_sem, 1);
        semop(sem_mutex, &wait_sem, 1);
        if (M[0] >= 240 && M[100 + id * 200 + 198] == 0) { // 3:00pm and no pending orders
            semop(sem_mutex, &signal_sem, 1);
            break;
        }
        int fr = M[base + 199]; // Food ready
        int po = M[base + 198]; // Pending orders
        if (fr) {
            M[base + 199] = 0;
            semop(sem_mutex, &signal_sem, 1);
            printf("[%02d:%02d %s] %sWaiter %c: Serving food to Customer %d\n",
                   M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", "\t" + (id ? 1 : 0) * (id - 1), name, fr);
            struct sembuf customer_signal = {fr - 1, 1, 0}; // Signal customer fr-1
            semop(sem_customer, &customer_signal, 1); // Corrected semop call
        } else if (po) {
            int front = M[base];
            int cust_id = M[front], count = M[front + 1];
            M[base] = front + 2;
            M[base + 198]--;
            semop(sem_mutex, &signal_sem, 1);

            printf("[%02d:%02d %s] %sWaiter %c: Placing order for Customer %d (count = %d)\n",
                   M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", "\t" + (id ? 1 : 0) * (id - 1), name, cust_id, count);

            int curr_time = M[0];
            usleep(MINUTE); // 1 minute to take order
            update_time(curr_time, 1);

            semop(sem_mutex, &wait_sem, 1);
            int back = M[1101];
            M[back] = id; M[back + 1] = cust_id; M[back + 2] = count;
            M[1101] = back + 3;
            M[3]++;
            semop(sem_mutex, &signal_sem, 1);
            semop(sem_cook, &signal_sem, 1); // Signal cook
        } else {
            semop(sem_mutex, &signal_sem, 1);
        }
    }
    printf("[%02d:%02d %s] %sWaiter %c leaving (no more customer to serve)\n",
           M[0] / 60 + 11, M[0] % 60, M[0] < 120 ? "am" : "pm", "\t" + (id ? 1 : 0) * (id - 1), name);
    exit(0);
}

int main() {
    init_ipc();
    for (int i = 0; i < 5; i++) {
        if (fork() == 0) {
            wmain(i);
        }
    }
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);
    return 0;
}