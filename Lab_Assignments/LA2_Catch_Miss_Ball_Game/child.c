#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int n;
int my_index;
int my_child_num;
pid_t *child_pids;
int playing = 1;
int last_catch = 0;


void print_status()
{
    

    if (my_index == 1)
    {
        printf("    "); // Initial spacing for first child
        fflush(stdout);
    }

    if (!playing)
    {
        printf("        "); // 8 spaces for empty slot
        fflush(stdout);
    }
    else if (last_catch == 1)
    {
        printf("CATCH    ");
        fflush(stdout);
        playing = 1;
        last_catch = 0;
    }
    else if (last_catch == -1)
    {
        printf("MISS     ");
        fflush(stdout);
        playing = 0;
    }
    else
    {
        printf("....    ");
        fflush(stdout);
    }

    if (my_index == n)
    {
        int width = n * 8 + 4 ;  // Adjust width according to number of children
        char separator[width + 1];
        for (int i = 0; i < width; i++)
        {
            separator[i] = '-';
        }
        separator[width] = '\0';
        printf("|\n+%s+\n", separator); // Dynamically print separator based on child size
        fflush(stdout);
    }
    fflush(stdout);
}

void signal_handler(int signo)
{
    if (signo == SIGUSR2)
    {
        double rand_val = (double)rand() / RAND_MAX;
        if (rand_val < 0.8)
        {
            last_catch = 1;
            kill(getppid(), SIGUSR1);
        }
        else
        {
            last_catch = -1;
            kill(getppid(), SIGUSR2);
        }
    }
    else if (signo == SIGUSR1)
    {
       
        print_status();
        if (my_index < n)
        {
            kill(child_pids[my_index], SIGUSR1);
        }
        else
        {
            FILE *fp = fopen("dummycpid.txt", "r");
            pid_t dummy_pid;
            fscanf(fp, "%d", &dummy_pid);
            fclose(fp);
            kill(dummy_pid, SIGINT);
        }
    }
    else if (signo == SIGINT)
    {
        if (playing)
        {
            printf("+++ Child %d: Yay! I am the winner!\n", my_index);
            fflush(stdout);
        }
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <index>\n", argv[0]);
        exit(1);
    }
    my_index = atoi(argv[1]);
    srand(time(NULL) + my_index);
    // printf("\t%d", my_index);
    fflush(stdout);

    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    sleep(1);
   

    FILE *fp = fopen("childpid.txt", "r");
    fscanf(fp, "%d", &n);
    child_pids = malloc((n + 1) * sizeof(pid_t));

    for (int i = 0; i < n; i++)
    {
        fscanf(fp, "%d", &child_pids[i]);
    }
    fclose(fp);

    // printf("%d\t", my_index);
    // fflush(stdout);

    while (1)
    {
        pause();
    }

    return 0;
}