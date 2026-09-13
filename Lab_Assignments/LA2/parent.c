#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

int n;              // number of children
pid_t *child_pids;  // array to store child PIDs
int *child_status;  // array to track child status (1: playing, 0: out)
int curr_child = 0; // index of current child
int remaining = 0;  // number of children still playing
volatile sig_atomic_t signal_received = 0;
volatile sig_atomic_t catch_status = 0;

int flag=0;

void signal_handler(int signo)
{
    signal_received = 1;
    if (signo == SIGUSR1)
        catch_status = 1;
    else if (signo == SIGUSR2)
        catch_status = 0;

    flag=1;

}

int find_next_player(int current)
{
    int next = (current + 1) % n;
    while (!child_status[next] && remaining > 1)
    {
        next = (next + 1) % n;
    }
    return next;
}

void printSeparator(int n)
{
    int width = n * 8 + 4 ;  // Adjust width according to number of children
        char separator[width + 1];
        for (int i = 0; i < width; i++)
        {
            separator[i] = '-';
        }
        separator[width] = '\0';
        printf("\n+%s+", separator); // Dynamically print separator based on child size
        fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <number_of_children>\n", argv[0]);
        exit(1);
    }

    n = atoi(argv[1]);
    if (n <= 0)
    {
        fprintf(stderr, "Number of children must be positive\n");
        fflush(stdout);
        exit(1);
    }

    child_pids = malloc(n * sizeof(pid_t));
    child_status = malloc(n * sizeof(int));

    // Set up signal handlers
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    // Create child processes and write PIDs to file
    FILE *fp = fopen("childpid.txt", "w");
    fprintf(fp, "%d\n", n);

    printf("Parent: %d child processes created\n", n);
    printf("Parent: Waiting for child processes to read child database\n\n");
    fflush(stdout);

    for (int i = 0; i < n; i++)
    {
        child_pids[i] = fork();
        if (child_pids[i] == 0)
        {
            char index_str[10];
            fflush(stdout);
            sprintf(index_str, "%d", i + 1);
            execl("./child", "child", index_str, NULL);
            exit(1);
        }
        fprintf(fp, "%d\n", child_pids[i]);
        child_status[i] = 1; // Initially all children are playing
    }
    fclose(fp);
    remaining = n;

    for(int i=1;i<=n;i++)
    {
        printf("\t%d",i);
    }

    printSeparator(n);

    sleep(2); // Give children time to read the file

    printf("\n");
    fflush(stdout);

    // Game loop
    while (remaining > 1)
    {
        // Create dummy process for synchronization
        pid_t dummy_pid = fork();
        if (dummy_pid == 0)
        {
            execl("./dummy", "dummy", NULL);
            exit(1);
        }

        // Write dummy PID to file
        fp = fopen("dummycpid.txt", "w");
        fprintf(fp, "%d\n", dummy_pid);
        fclose(fp);

        // Send ball to current child
        kill(child_pids[curr_child], SIGUSR2);
        signal_received = 0;

        // Wait for child's response
        while (flag!=1)
            {
                //Infinite looping mimicing pause()
            }
        flag=0;


        // Update child's status if they missed
        if (!catch_status)
        {
            child_status[curr_child] = 0;
            remaining--;
        }

        // Trigger status printing
        kill(child_pids[0], SIGUSR1);

        // Wait for dummy to be killed
        waitpid(dummy_pid, NULL, 0);

        // Find next player
        if (remaining > 1)
        {
            curr_child = find_next_player(curr_child);
        }
    }

    // Game over - send SIGINT to all children
    for (int i = 0; i < n; i++)
    {
        kill(child_pids[i], SIGINT);
        waitpid(child_pids[i], NULL, 0);
    }


    free(child_pids);
    free(child_status);
    return 0;
}
