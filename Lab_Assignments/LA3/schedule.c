#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Maximum limits for various data structures
#define MAX_BURSTS 20      // Maximum number of CPU/IO bursts per process
#define MAX_PROCESSES 1000 // Maximum number of processes in the simulation
#define MAX_EVENTS 10000   // Maximum number of events that can be tracked
#define INF 1000000000     // Infinite quantum for FCFS scheduling

// Verbose printing macro for debugging and detailed output
#ifdef VERBOSE
#define VERBOSE_PRINT(...) printf(__VA_ARGS__)
#else
#define VERBOSE_PRINT(...)
#endif

// Enum to represent different process states
typedef enum
{
    NEW,       // Process just created
    READY,     // Process waiting in ready queue
    RUNNING,   // Process currently executing
    WAITING,   // Process waiting for I/O
    TERMINATED // Process has completed execution
} ProcessState;

// Process Control Block (PCB) structure to store process-specific information
typedef struct
{
    int id;                 // Unique process identifier
    int arrival_time;       // Time when process arrives
    int bursts[MAX_BURSTS]; // Array of CPU and I/O burst times
    int num_bursts;         // Total number of bursts
    int current_burst;      // Current burst being executed
    int completion_time;    // Time when process completes
    int waiting_time;       // Total time spent waiting
    int running_time;
    int time;           // Total CPU execution time
    ProcessState state; // Current state of the process
} PCB;

// Enum to represent different event types in the simulation
typedef enum
{
    ARRIVAL,        // Process arrives
    CPU_COMPLETION, // Process completes CPU burst
    IO_COMPLETION,  // Process completes I/O burst
    CPU_TIMEOUT     // Process is preempted due to time quantum expiry
} EventType;

// Event structure to track and manage simulation events
typedef struct
{
    int time;        // Time of event occurrence
    int process_idx; // Index of process associated with event
    EventType type;  // Type of event
} Event;

// FIFO Queue structure for managing ready queue
typedef struct
{
    int *data;          // Array to store queue elements
    int front, rear;    // Front and rear indices
    int size, capacity; // Current size and total capacity
} Queue;

// Min-Heap based Event Queue structure
typedef struct
{
    Event *events;      // Array to store events
    int size, capacity; // Current size and total capacity
} EventQueue;

// Global variables for simulation
PCB processes[MAX_PROCESSES]; // Array of process control blocks
Queue ready_queue;            // Ready queue for process scheduling
EventQueue event_queue;       // Event queue for managing simulation events
int num_processes;            // Total number of processes
int current_time;             // Current simulation time
int cpu_busy;                 // Index of currently running process
int cpu_idle_time;            // Total time CPU remains idle

// Function prototypes for queue operations
void init_queue(Queue *q, int capacity);
void enqueue(Queue *q, int value);
int dequeue(Queue *q);
int is_empty_queue(Queue *q);

// Function prototypes for event queue operations
void init_event_queue(EventQueue *eq, int capacity);
void add_event(int time, int process_idx, EventType type);
Event get_next_event();
void swap_events(Event *a, Event *b);
int compare_events(Event *a, Event *b);

// Function prototypes for scheduling and event handling
void handle_event(Event e, int quantum);
void schedule(int quantum);
void print_statistics(int quantum);

// Queue implementation functions (detailed comments in code)
void init_queue(Queue *q, int capacity)
{
    // Initialize queue with given capacity
    q->data = (int *)malloc(capacity * sizeof(int));
    q->front = q->rear = -1;
    q->size = 0;
    q->capacity = capacity;
}

void enqueue(Queue *q, int value)
{
    // Add element to the rear of the queue
    if (q->size == q->capacity)
        return;
    if (q->size == 0)
        q->front = q->rear = 0;
    else
        q->rear = (q->rear + 1) % q->capacity;
    q->data[q->rear] = value;
    q->size++;
}

int dequeue(Queue *q)
{
    // Remove and return element from the front of the queue
    if (q->size == 0)
        return -1;
    int value = q->data[q->front];
    if (q->size == 1)
        q->front = q->rear = -1;
    else
        q->front = (q->front + 1) % q->capacity;
    q->size--;
    return value;
}

int is_empty_queue(Queue *q)
{
    // Check if queue is empty
    return q->size == 0;
}

// Event queue implementation functions
void init_event_queue(EventQueue *eq, int capacity)
{
    // Initialize event queue with given capacity
    eq->events = (Event *)malloc(capacity * sizeof(Event));
    eq->size = 0;
    eq->capacity = capacity;
}

void add_event(int time, int process_idx, EventType type)
{
    // printf("We are here in this func\n");
    // Add event to event queue and maintain min-heap property
    Event e = {time, process_idx, type};

    if (event_queue.size == event_queue.capacity)
        return;
    event_queue.events[event_queue.size] = e;
    int i = event_queue.size++;
    while (i > 0)
    {
        int parent = (i - 1) / 2;
        if (compare_events(&event_queue.events[i], &event_queue.events[parent]) < 0)
        {
            swap_events(&event_queue.events[i], &event_queue.events[parent]);
            i = parent;
        }
        else
            break;
    }
}

Event get_next_event()
{
    // Extract and return the minimum event from event queue
    Event top = event_queue.events[0];
    event_queue.events[0] = event_queue.events[--event_queue.size];
    int i = 0;
    while (1)
    {
        int left = 2 * i + 1, right = 2 * i + 2, smallest = i;
        if (left < event_queue.size && compare_events(&event_queue.events[left], &event_queue.events[smallest]) < 0)
            smallest = left;
        if (right < event_queue.size && compare_events(&event_queue.events[right], &event_queue.events[smallest]) < 0)
            smallest = right;
        if (smallest != i)
        {
            swap_events(&event_queue.events[i], &event_queue.events[smallest]);
            i = smallest;
        }
        else
            break;
    }
    return top;
}

void swap_events(Event *a, Event *b)
{
    // Swap two events
    Event temp = *a;
    *a = *b;
    *b = temp;
}

int compare_events(Event *a, Event *b)
{
    // Compare events first by time, then by process ID
    if (a->time != b->time)
        return a->time - b->time;
    if (a->time != b->time)
        return a->time - b->time;
    return processes[a->process_idx].id - processes[b->process_idx].id;
}

void handle_event(Event e, int quantum)
{
    // Handle different types of events in the simulation
    PCB *p = &processes[e.process_idx];
    current_time = e.time;

    VERBOSE_PRINT("%d : Handling event for Process %d\n", current_time, p->id);

    switch (e.type)
    {
    // p->completion_time = current_time;
    case ARRIVAL:
    case IO_COMPLETION:
        // Process arrives or completes I/O
        VERBOSE_PRINT("%d : Process %d joins ready queue\n", current_time, p->id);
        current_time += p->bursts[p->current_burst];
        enqueue(&ready_queue, e.process_idx);
        p->state = READY;
        p->current_burst = p->current_burst+1;
        break;

    case CPU_COMPLETION:
        if (p->current_burst == p->num_bursts)
        { // Process completes its entire execution
            p->completion_time = current_time;
            int turnaround_time = p->completion_time - p->arrival_time;
            int wait_time = turnaround_time - p->running_time;

            // Print process exit details
            printf("%d : Process %d exits. Turnaround time = %d (%d%%), Wait time = %d\n",
                   current_time, p->id, turnaround_time,
                   (turnaround_time * 100) / p->running_time, wait_time);

            printf("%d : Process %d exits. comletion time = %d\n", current_time, p->id, p->completion_time);
            printf("%d : Process %d exits. arrival_time =%d \n", current_time, p->id, p->arrival_time);
            printf("%d : Process %d exits. running time =%d \n\n", current_time, p->id, p->running_time);

            p->state = TERMINATED;
            cpu_busy = -1;
            break;
        }
        else
        {
            // Process completes its entire execution
            p->completion_time = current_time;
            int turnaround_time = p->completion_time - p->arrival_time;
            // int wait_time = turnaround_time - p->running_time;

            // Print process exit details
            // printf("%d : Process %d exits. Turnaround time = %d (%d%%), Wait time = %d\n",
            //        current_time, p->id, turnaround_time,
            //        (turnaround_time * 100) / p->running_time, wait_time);

            // printf("%d : Process %d exits. comletion time = %d\n", current_time, p->id, p->completion_time);
            // printf("%d : Process %d exits. arrival_time =%d \n", current_time, p->id, p->arrival_time);
            // printf("%d : Process %d exits. running time =%d \n\n", current_time, p->id, p->running_time);

            // add_event(current_time,p->id,ready_queue);

            // enqueue(&ready_queue,p->id);

            p->current_burst+=1;
            add_event(current_time,p->id,IO_COMPLETION);


            p->state = READY;
            cpu_busy = -1;
            break;

        }
    case CPU_TIMEOUT:
        // Process is preempted due to time quantum expiry
        VERBOSE_PRINT("%d : Process %d is preempted\n", current_time, p->id);
        enqueue(&ready_queue, e.process_idx);
        cpu_busy = -1;
        break;
    }

    // Update waiting time for processes in ready queue
    for (int i = 0; i < ready_queue.size; i++)
    {
        int idx = ready_queue.data[(ready_queue.front + i) % ready_queue.capacity];
        processes[idx].waiting_time++;
    }

    // Schedule next process if CPU is idle
    if (cpu_busy == -1 && !is_empty_queue(&ready_queue))
    {
        int next = dequeue(&ready_queue);
        PCB *next_process = &processes[next];
        next_process->state = RUNNING;
        cpu_busy = next;

        // Determine run time based on time quantum and burst time
        int run_time = (quantum < next_process->bursts[next_process->current_burst])
                           ? quantum
                           : next_process->bursts[next_process->current_burst];

        // printf("%d(%d) : [%d-%d]\t", next_process->id, next_process->current_burst, current_time, current_time + run_time);

        if (quantum >= next_process->bursts[next_process->current_burst])
        {
            // printf("The value of curret time is %d\n",run_time);
            add_event(current_time + run_time, next, CPU_COMPLETION);
        }
        else
        {
            next_process->bursts[next_process->current_burst] -= run_time;
            add_event(current_time + run_time, next, CPU_TIMEOUT);
        }
    }
    else if (cpu_busy == -1)
    {
        cpu_idle_time++;
    }
}

void schedule(int quantum)
{
    // Main scheduling function to simulate CPU scheduling
    current_time = 0;
    cpu_idle_time = 0;
    cpu_busy = -1;

    // Print scheduling algorithm details
    printf("\n**** %s Scheduling ****\n", (quantum == INF) ? "FCFS" : "RR");
    if (quantum != INF)
        printf("Time Quantum = %d\n", quantum);

    // Process events until event queue is empty
    while (event_queue.size > 0)
    {
        Event e = get_next_event();

        // Handle idle time between events
        if (cpu_busy == -1 && e.time > current_time)
        {
            cpu_idle_time += e.time - current_time;
            current_time = e.time;
        }
        current_time = e.time;
        handle_event(e, quantum);

        // Process all events occurring at the same time
        while (event_queue.size > 0 && event_queue.events[0].time == current_time)
        {
            Event next_event = get_next_event();
            handle_event(next_event, quantum);
        }
    }

    // Print simulation statistics
    print_statistics(quantum);
}

void print_statistics(int quantum)
{
    // Calculate and print overall simulation statistics
    double avg_wait_time = 0;
    int total_turnaround_time = 0;
    int total_running_time = 0;

    for (int i = 0; i < num_processes; i++)
    {
        PCB *p = &processes[i];
        int turnaround_time = p->completion_time - p->arrival_time;
        int wait_time = turnaround_time - p->running_time;

        total_turnaround_time += turnaround_time;
        avg_wait_time += wait_time;
        total_running_time += p->running_time;
    }

    avg_wait_time /= num_processes;

    // Print performance metrics
    printf("Average wait time = %.2f\n", avg_wait_time);
    printf("Total turnaround time = %d\n", total_turnaround_time);
    printf("CPU idle time = %d\n", cpu_idle_time);
    printf("CPU utilization = %.2f%%\n",
           100.0 * (total_turnaround_time - cpu_idle_time) / total_turnaround_time);
}

int main()
{
    // Read process details from input file
    FILE *fp = fopen("proc.txt", "r");
    if (!fp)
    {
        perror("Error opening proc.txt");
        return 1;
    }

    // Read number of processes
    fscanf(fp, "%d", &num_processes);
    // printf("Number of processes: %d\n", num_processes);
    for (int i = 0; i < num_processes; i++)
    {
        // PCB *p = &processes[i];
        // fscanf(fp, "%d ,  %d", &processes[i].id, &processes[i].arrival_time);
        fscanf(fp, "%d", &processes[i].id);
        fscanf(fp, "%d", &processes[i].arrival_time);

        // printf("%d and %d\n", processes[i].id, processes[i].arrival_time);

        // Read burst times for the process
        int burst, j = 0;
        while (fscanf(fp, "%d", &burst) && burst != -1)
        {
            // printf("Burst read is %d\n", burst);
            processes[i].bursts[j++] = burst;
        }

        // Initialize process metadata
        processes[i].num_bursts = j;
        processes[i].running_time = 0;
        for (int k = 0; k < j; k++)
            processes[i].running_time += processes[i].bursts[k];
        processes[i].waiting_time = 0;
        processes[i].current_burst = 0;
        processes[i].state = NEW;

        add_event(processes[i].arrival_time, i, ARRIVAL);
    }

    // printf("Proecesses\n");
    // for (int i = 0; i < num_processes; i++)
    // {
    //     printf("The value of the processes[i].num_bursts is %d\n", processes[i].num_bursts);
    //     for (int j = 0; j < processes[i].num_bursts; j++)
    //     {
    //         printf("%d ", processes[i].bursts[j]);
    //     }
    //     printf("\n");
    // }

    fclose(fp);

    // Initialize ready and event queues
    init_queue(&ready_queue, MAX_PROCESSES);

    init_event_queue(&event_queue, MAX_EVENTS);
    for (int i = 0; i < num_processes; i++)
    {
        add_event(processes[i].arrival_time, i, ARRIVAL);
    }

    // Run scheduling simulations for different algorithms/quantum values
    // schedule(INF); // FCFS

    // for (int i = 0; i < num_processes; i++)
    // {
    //     add_event(processes[i].arrival_time, i, ARRIVAL);
    // }
    // // Uncomment following lines to simulate RR with different quantum values
    // schedule(10);  // RR q = 10
    
    // for (int i = 0; i < num_processes; i++)
    // {
    //     add_event(processes[i].arrival_time, i, ARRIVAL);
    // }

    // schedule(5);   // RR q = 5

    int val;
    printf("Enter the value 1[FCFS] , 2[RR-10] and 3[RR-5] : ");
    scanf("%d",&val);

    if(val==1)val=1e9;
    else if(val==2)val=10;
    else val=5;

    schedule(val);


    // Free dynamically allocated memory
    free(ready_queue.data);
    free(event_queue.events);
    return 0;
}
