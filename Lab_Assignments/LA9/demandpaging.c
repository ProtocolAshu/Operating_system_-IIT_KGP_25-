// OS Lab Assignment 9 
// Name -> Animesh Kumar
// Roll_no -> 22CS30009


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Configurable constants
#define PAGE_SIZE 4096 // 4 KB pages
#define TOTAL_FRAMES 16384 // 64 MB memory
#define RESERVED_FRAMES 4096 // 16 MB reserved
#define USABLE_FRAMES 12288 // 48 MB available frames
#define TOTAL_PAGES 2048 // Pages per process
#define ESSENTIAL_PAGES 10 // Essential pages per process

// Verbose mode handling
#ifdef VERBOSE
 FILE* verbose_output = NULL;
 #define VERBOSE_PRINT(...) do { \
 if (verbose_output) { \
 fprintf(verbose_output, __VA_ARGS__); \
 fflush(verbose_output); \
 } \
 } while(0)
#else
 #define VERBOSE_PRINT(...)
#endif

// Bit manipulation macros
#define SET_VALID(entry) ((entry) |= (1U << 15))
#define CLEAR_VALID(entry) ((entry) &= ~(1U << 15))
#define IS_VALID(entry) ((entry) & (1U << 15))
#define GET_FRAME(entry) ((entry) & 0x7FFF)
#define CREATE_ENTRY(frame) ((frame) | (1U << 15))

// Process structure
typedef struct {
 int process_id;
 int array_size;
 int *search_indices;
 unsigned short *page_table;
 int current_search;
 int frames_allocated;
 bool is_completed;
 bool is_swapped_out;
} Process;

// Queue structure for FIFO
typedef struct {
 int *array;
 int front, rear, size, capacity;
} Queue;

// Global variables
Process *processes = NULL;
int num_processes = 0, num_searches = 0;
int *free_frames = NULL;
int num_free_frames = 0;
int total_page_accesses = 0, total_page_faults = 0, total_swaps = 0;
int min_active_processes = 0;

Queue *ready_queue = NULL, *swapped_out_queue = NULL;

// Queue functions
Queue* create_queue(int capacity) {
 Queue *q = malloc(sizeof(Queue));
 if (!q) { perror("Queue allocation failed"); exit(1); }
 q->array = malloc(capacity * sizeof(int));
 if (!q->array) { perror("Queue array allocation failed"); exit(1); }
 q->front = q->rear = -1;
 q->size = 0;
 q->capacity = capacity;
 return q;
}

void enqueue(Queue *q, int item) {
 if (q->size == q->capacity) return;
 if (q->front == -1) q->front = 0;
 q->rear = (q->rear + 1) % q->capacity;
 q->array[q->rear] = item;
 q->size++;
}

int dequeue(Queue *q) {
 if (q->size == 0) return -1;
 int item = q->array[q->front];
 q->front = (q->front + 1) % q->capacity;
 q->size--;
 if (q->size == 0) q->front = q->rear = -1;
 return item;
}

bool is_empty(Queue *q) {
 return q->size == 0;
}

// Memory management functions
void initialize_memory() {
 free_frames = malloc(USABLE_FRAMES * sizeof(int));
 if (!free_frames) { perror("Memory allocation failed for free frames"); exit(1); }
 for (int i = 0; i < USABLE_FRAMES; i++) free_frames[i] = i;
 num_free_frames = USABLE_FRAMES;

 processes = malloc(num_processes * sizeof(Process));
 if (!processes) { perror("Memory allocation failed for processes"); exit(1); }
 memset(processes, 0, num_processes * sizeof(Process));

 ready_queue = create_queue(num_processes);
 swapped_out_queue = create_queue(num_processes);
}

int allocate_frame() {
 if (num_free_frames == 0) return -1;
 return free_frames[--num_free_frames];
}

void free_process_frames(int pid) {
 Process *p = &processes[pid];
 for (int i = 0; i < TOTAL_PAGES; i++) {
 if (IS_VALID(p->page_table[i])) {
 free_frames[num_free_frames++] = GET_FRAME(p->page_table[i]);
 CLEAR_VALID(p->page_table[i]);
 }
 }
 p->frames_allocated = 0;
}

bool load_page(Process *p, int page_index) {
 if (IS_VALID(p->page_table[page_index])) return true;

 total_page_faults++;
 int frame = allocate_frame();
 if (frame == -1) return false;
 p->page_table[page_index] = CREATE_ENTRY(frame);
 p->frames_allocated++;
 return true;
}

void perform_binary_search(Process *p) {
 int s = p->array_size;
 int k = p->search_indices[p->current_search];
 VERBOSE_PRINT("\tSearch %d by Process %d\n", p->current_search + 1, p->process_id);

 int L = 0, R = s - 1;
 while (L < R) {
 total_page_accesses++;
 int M = (L + R) / 2;
 int page_index = M / (PAGE_SIZE / sizeof(int)) + ESSENTIAL_PAGES;
 
 if (!load_page(p, page_index)) {
 printf("+++ Swapping out process %3d [%d active processes]\n",
 p->process_id, num_processes - total_swaps - 1);
 total_swaps++;
 free_process_frames(p->process_id);
 p->is_swapped_out = true;
 enqueue(swapped_out_queue, p->process_id);
 
 int active = num_processes - total_swaps - (num_processes - ready_queue->size - total_swaps);
 if (total_swaps == 1 || active < min_active_processes) {
 min_active_processes = active;
 }
 return;
 }
 
 if (k <= M) R = M;
 else L = M + 1;
 }
 
 p->current_search++;
 if (p->current_search >= num_searches) {
 p->is_completed = true;
 free_process_frames(p->process_id);
 }
}

void read_input_file() {
 FILE *fp = fopen("search.txt", "r");
 if (!fp) { perror("Error opening search.txt"); exit(1); }
 
 if (fscanf(fp, "%d %d", &num_processes, &num_searches) != 2) {
 perror("Error reading number of processes and searches");
 fclose(fp);
 exit(1);
 }

 initialize_memory();

 for (int i = 0; i < num_processes; i++) {
 Process *p = &processes[i];
 p->process_id = i;
 p->page_table = calloc(TOTAL_PAGES, sizeof(unsigned short));
 if (!p->page_table) { perror("Page table allocation failed"); exit(1); }
 p->search_indices = malloc(num_searches * sizeof(int));
 if (!p->search_indices) { perror("Search indices allocation failed"); exit(1); }
 p->is_completed = p->is_swapped_out = false;

 if (fscanf(fp, "%d", &p->array_size) != 1) {
 perror("Error reading array size");
 fclose(fp);
 exit(1);
 }
 
 for (int j = 0; j < num_searches; j++) {
 if (fscanf(fp, "%d", &p->search_indices[j]) != 1) {
 perror("Error reading search indices");
 fclose(fp);
 exit(1);
 }
 }

 for (int j = 0; j < ESSENTIAL_PAGES; j++) {
 int frame = allocate_frame();
 if (frame == -1) { perror("Not enough frames for essential pages"); fclose(fp); exit(1); }
 p->page_table[j] = CREATE_ENTRY(frame);
 p->frames_allocated++;
 }
 
 enqueue(ready_queue, i);
 }
 
 fclose(fp);
 printf("+++ Simulation data read from file\n");
 printf("+++ Kernel data initialized\n");
}

void run_simulation() {
 int completed_processes = 0;
 min_active_processes = num_processes;

 while (completed_processes < num_processes) {
 int pid = dequeue(ready_queue);
 
 if (pid == -1) {
 if (is_empty(swapped_out_queue)) break;
 continue; // Wait for a process to complete
 }

 Process *p = &processes[pid];
 
 if (!p->is_completed && !p->is_swapped_out) {
 perform_binary_search(p);
 
 if (p->is_completed) {
 completed_processes++;
 if (!is_empty(swapped_out_queue) && num_free_frames >= ESSENTIAL_PAGES) {
 int swap_pid = dequeue(swapped_out_queue);
 Process *q = &processes[swap_pid];
 
 for (int j = 0; j < ESSENTIAL_PAGES; j++) {
 int frame = allocate_frame();
 if (frame == -1) { perror("Unexpected frame allocation failure"); exit(1); }
 q->page_table[j] = CREATE_ENTRY(frame);
 q->frames_allocated++;
 }
 
 q->is_swapped_out = false;
 printf("+++ Swapping in process %3d [%d active processes]\n",
 swap_pid, num_processes - completed_processes - swapped_out_queue->size);
 perform_binary_search(q); // Restart the interrupted search
 
 if (!q->is_completed && !q->is_swapped_out) {
 enqueue(ready_queue, swap_pid);
 }
 }
 } else if (!p->is_swapped_out) {
 enqueue(ready_queue, pid);
 }
 }
 }
}

void print_statistics() {
 printf("+++ Page access summary\n");
 printf("\tTotal number of page accesses = %d\n", total_page_accesses);
 printf("\tTotal number of page faults = %d\n", total_page_faults);
 printf("\tTotal number of swaps = %d\n", total_swaps);
 printf("\tDegree of multiprogramming = %d\n", min_active_processes);
}

int main() {
 FILE* output_file = fopen("output.txt", "w");
 if (!output_file) { perror("Error opening output.txt"); return 1; }
 stdout = output_file;

 #ifdef VERBOSE
 verbose_output = fopen("verboseoutput.txt", "w");
 stdout=verbose_output; 
 if (!verbose_output) { perror("Error opening verboseoutput.txt"); fclose(output_file); return 1; }
 #endif

 read_input_file();
 run_simulation();
 print_statistics();

 fclose(output_file);
 #ifdef VERBOSE
 fclose(verbose_output);
 #endif

 for (int i = 0; i < num_processes; i++) {
 free(processes[i].page_table);
 free(processes[i].search_indices);
 }
 free(processes);
 free(free_frames);
 free(ready_queue->array);
 free(ready_queue);
 free(swapped_out_queue->array);
 free(swapped_out_queue);
 
 return 0;
}