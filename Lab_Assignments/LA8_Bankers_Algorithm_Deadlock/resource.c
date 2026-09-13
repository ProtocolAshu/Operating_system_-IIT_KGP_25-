#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

/* Constants for resource request types */
#define REQUEST_RELEASE 0
#define REQUEST_ADDITIONAL 1  
#define REQUEST_QUIT 2

/* Scale factor for delays */
#define DELAY_SCALE 1000

/* Input directory path */
#define INPUT_DIR "input"

/* Global variables for system configuration */
int m;  // Number of resource types
int n;  // Number of threads
int active_threads;  // Counter for active user threads

/* Synchronization primitives */
pthread_barrier_t BOS;  // Barrier for Beginning Of Session
pthread_barrier_t REQB;  // Barrier for request synchronization
pthread_barrier_t *ACKB;  // Array of barriers for per-thread acknowledgments
pthread_mutex_t rmtx;  // Mutex for request data access
pthread_mutex_t pmtx;  // Mutex for output printing
pthread_cond_t *cv;  // Array of condition variables for thread signaling
pthread_mutex_t *cmtx;  // Array of mutexes for condition variables

/* Shared memory for request communication */
int *request_data;  // [thread_id, request_type, req[0], req[1], ..., req[m-1]]

/* System resource state */
int *AVAILABLE;  // Available resources
int **ALLOCATION;  // Current allocation matrix
int **NEED;  // Maximum need matrix

/* Queue for pending requests */
typedef struct request_node {
    int thread_id;
    int *request;
    struct request_node *next;
} request_node_t;

typedef struct {
    request_node_t *head;
    request_node_t *tail;
} request_queue_t;

request_queue_t pending_requests;

/* Forward declarations */
void *master_thread(void *arg);
void *user_thread(void *arg);
bool is_safe_state(int thread_id, int *request);
void initialize_queue();
void enqueue_request(int thread_id, int *request);
bool process_pending_requests();
bool can_allocate(int thread_id, int *request);
void handle_release_request(int thread_id, int *request);
void print_waiting_threads();
void format_thread_id(int id, char *buffer);
void read_system_configuration();
void read_thread_max_needs(int thread_id, int *needs);
int get_request_type(int *request);

int main() {
    pthread_t *threads;
    int i;
    
    /* Read system configuration */
    read_system_configuration();
    
    /* Initialize active threads counter */
    active_threads = n;
    
    /* Initialize synchronization primitives */
    pthread_barrier_init(&BOS, NULL, n + 1);
    pthread_barrier_init(&REQB, NULL, 2);
    
    ACKB = (pthread_barrier_t *)malloc(n * sizeof(pthread_barrier_t));
    cv = (pthread_cond_t *)malloc(n * sizeof(pthread_cond_t));
    cmtx = (pthread_mutex_t *)malloc(n * sizeof(pthread_mutex_t));
    
    for (i = 0; i < n; i++) {
        pthread_barrier_init(&ACKB[i], NULL, 2);
        pthread_cond_init(&cv[i], NULL);
        pthread_mutex_init(&cmtx[i], NULL);
    }
    
    pthread_mutex_init(&rmtx, NULL);
    pthread_mutex_init(&pmtx, NULL);
    
    /* Initialize shared memory for request communication */
    request_data = (int *)malloc((m + 2) * sizeof(int));
    
    /* Initialize the request queue */
    initialize_queue();
    
    /* Create user threads */
    threads = (pthread_t *)malloc(n * sizeof(pthread_t));
    
    for (i = 0; i < n; i++) {
        int *thread_id = (int *)malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&threads[i], NULL, user_thread, (void *)thread_id);
    }
    
    /* Execute master thread logic */
    master_thread(NULL);
    
    /* Wait for all user threads to finish */
    for (i = 0; i < n; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Clean up */
    pthread_barrier_destroy(&BOS);
    pthread_barrier_destroy(&REQB);
    
    for (i = 0; i < n; i++) {
        pthread_barrier_destroy(&ACKB[i]);
        pthread_cond_destroy(&cv[i]);
        pthread_mutex_destroy(&cmtx[i]);
    }
    
    pthread_mutex_destroy(&rmtx);
    pthread_mutex_destroy(&pmtx);
    
    free(ACKB);
    free(cv);
    free(cmtx);
    free(request_data);
    
    for (i = 0; i < n; i++) {
        free(ALLOCATION[i]);
        free(NEED[i]);
    }
    
    free(ALLOCATION);
    free(NEED);
    free(AVAILABLE);
    free(threads);
    
    return 0;
}

void *master_thread(void *arg) {
    int i, thread_id, request_type;
    int *request;
    
    /* Wait for all threads to be ready */
    pthread_barrier_wait(&BOS);
    
    /* Process requests until all user threads have finished */
    while (active_threads > 0) {
        /* Wait for a request */
        pthread_barrier_wait(&REQB);
        
        /* Extract request information */
        thread_id = request_data[0];
        request_type = request_data[1];
        
        request = (int *)malloc(m * sizeof(int));
        for (i = 0; i < m; i++) {
            request[i] = request_data[i + 2];
        }
        
        /* Acknowledge receipt of request */
        pthread_barrier_wait(&ACKB[thread_id]);
        
        /* Handle the request based on its type */
        pthread_mutex_lock(&pmtx);
        
        if (request_type != REQUEST_QUIT) {
            printf("Master thread stores resource request of thread %02d\n", thread_id);
        }
        
        if (request_type == REQUEST_RELEASE) {
            /* Handle release request - immediately update resources */
            handle_release_request(thread_id, request);
        } else if (request_type == REQUEST_QUIT) {
            /* Handle quit request - release all resources */
            for (i = 0; i < m; i++) {
                AVAILABLE[i] += ALLOCATION[thread_id][i];
                ALLOCATION[thread_id][i] = 0;
            }
            
            printf("Master thread releases resources of thread %02d\n", thread_id);
            active_threads--;
        } else if (request_type == REQUEST_ADDITIONAL) {
            /* Handle additional request */
            /* First, handle any release components */
            for (i = 0; i < m; i++) {
                if (request[i] < 0) {
                    AVAILABLE[i] += -request[i];
                    ALLOCATION[thread_id][i] += request[i];
                    request[i] = 0;
                }
            }
            
            /* Enqueue the additional request */
            enqueue_request(thread_id, request);
        }
        
        /* Print waiting threads */
        print_waiting_threads();
        
        printf("Master thread tries to grant pending requests\n");
        
        /* Process pending requests */
        process_pending_requests();
        
        /* Print waiting threads again after processing */
        print_waiting_threads();
        
        pthread_mutex_unlock(&pmtx);
    }
    
    return NULL;
}

void *user_thread(void *arg) {
    int thread_id = *((int *)arg);
    free(arg);
    
    char thread_file[100];
    char buffer[100];
    FILE *fp;
    int i, delay, request_type;
    char req_type_char;
    int *max_needs = (int *)malloc(m * sizeof(int));
    int *request = (int *)malloc(m * sizeof(int));
    
    /* Print thread creation message */
    pthread_mutex_lock(&pmtx);
    printf("\tThread %02d born\n", thread_id);
    pthread_mutex_unlock(&pmtx);
    
    /* Read thread's maximum needs */
    read_thread_max_needs(thread_id, max_needs);
    
    /* Format thread filename */
    sprintf(thread_file, "%s/thread%02d.txt", INPUT_DIR, thread_id);
    
    /* Open thread file */
    fp = fopen(thread_file, "r");
    if (!fp) {
        pthread_mutex_lock(&pmtx);
        printf("Error: Cannot open file %s\n", thread_file);
        pthread_mutex_unlock(&pmtx);
        exit(1);
    }
    
    /* Skip the first line (max needs) */
    fgets(buffer, sizeof(buffer), fp);
    
    /* Wait for all threads to be ready */
    pthread_barrier_wait(&BOS);
    
    /* Process requests from file */
    while (fgets(buffer, sizeof(buffer), fp)) {
        sscanf(buffer, "%d %c", &delay, &req_type_char);
        
        /* Sleep for the specified delay */
        usleep(delay * DELAY_SCALE);
        
        if (req_type_char == 'Q') {
            /* Handle quit request */
            request_type = REQUEST_QUIT;
            
            pthread_mutex_lock(&rmtx);
            
            /* Prepare request data */
            request_data[0] = thread_id;
            request_data[1] = request_type;
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %02d send resource request: type = QUIT\n", thread_id);
            pthread_mutex_unlock(&pmtx);
            
            /* Synchronize with master thread */
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACKB[thread_id]);
            
            pthread_mutex_unlock(&rmtx);
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %02d is going to quit\n", thread_id);
            pthread_mutex_unlock(&pmtx);
            
            free(max_needs);
            free(request);
            fclose(fp);
            return NULL;
        } else {
            /* Parse resource request */
            char *token = strtok(buffer, " ");
            token = strtok(NULL, " ");  /* Skip DELAY and R/Q */
            
            for (i = 0; i < m && token != NULL; i++) {
                token = strtok(NULL, " ");
                if (token != NULL) {
                    request[i] = atoi(token);
                }
            }
            
            /* Determine request type */
            request_type = get_request_type(request);
            
            pthread_mutex_lock(&rmtx);
            
            /* Prepare request data */
            request_data[0] = thread_id;
            request_data[1] = request_type;
            
            for (i = 0; i < m; i++) {
                request_data[i + 2] = request[i];
            }
            
            pthread_mutex_lock(&pmtx);
            printf("\tThread %02d send resource request: type = %s\n", 
                  thread_id, 
                  (request_type == REQUEST_RELEASE) ? "RELEASE" : "ADDITIONAL");
            pthread_mutex_unlock(&pmtx);
            
            /* Synchronize with master thread */
            pthread_barrier_wait(&REQB);
            pthread_barrier_wait(&ACKB[thread_id]);
            
            pthread_mutex_unlock(&rmtx);
            
            if (request_type == REQUEST_RELEASE) {
                /* For release requests, continue immediately */
                pthread_mutex_lock(&pmtx);
                printf("\tThread %02d is done with its resource release request\n", thread_id);
                pthread_mutex_unlock(&pmtx);
            } else {
                /* For additional requests, wait until granted */
                pthread_mutex_lock(&cmtx[thread_id]);
                pthread_cond_wait(&cv[thread_id], &cmtx[thread_id]);
                pthread_mutex_unlock(&cmtx[thread_id]);
                
                pthread_mutex_lock(&pmtx);
                printf("\tThread %02d is granted its last resource request\n", thread_id);
                pthread_mutex_unlock(&pmtx);
            }
        }
    }
    
    free(max_needs);
    free(request);
    fclose(fp);
    return NULL;
}

bool is_safe_state(int thread_id, int *request) {
#ifdef _DLAVOID
    int i, j;
    bool found;
    int *work = (int *)malloc(m * sizeof(int));
    bool *finish = (bool *)malloc(n * sizeof(bool));
    
    /* Initialize work and finish arrays */
    for (i = 0; i < m; i++) {
        work[i] = AVAILABLE[i] - request[i];
    }
    
    for (i = 0; i < n; i++) {
        finish[i] = false;
    }
    
    /* Temporarily allocate the requested resources */
    for (i = 0; i < m; i++) {
        ALLOCATION[thread_id][i] += request[i];
        NEED[thread_id][i] -= request[i];
    }
    
    /* Banker's algorithm */
    while (true) {
        found = false;
        
        for (i = 0; i < n; i++) {
            if (!finish[i]) {
                /* Check if thread i's needs can be satisfied */
                bool can_satisfy = true;
                for (j = 0; j < m; j++) {
                    if (NEED[i][j] > work[j]) {
                        can_satisfy = false;
                        break;
                    }
                }
                
                if (can_satisfy) {
                    /* Add thread i's allocated resources to work */
                    for (j = 0; j < m; j++) {
                        work[j] += ALLOCATION[i][j];
                    }
                    
                    finish[i] = true;
                    found = true;
                }
            }
        }
        
        if (!found) {
            break;
        }
    }
    
    /* Check if all threads can finish */
    bool safe = true;
    for (i = 0; i < n; i++) {
        if (!finish[i]) {
            safe = false;
            break;
        }
    }
    
    /* Restore original state */
    for (i = 0; i < m; i++) {
        ALLOCATION[thread_id][i] -= request[i];
        NEED[thread_id][i] += request[i];
    }
    
    free(work);
    free(finish);
    
    return safe;
#else
    /* If deadlock avoidance is not enabled, always return true */
    return true;
#endif
}

void initialize_queue() {
    pending_requests.head = NULL;
    pending_requests.tail = NULL;
}

void enqueue_request(int thread_id, int *request) {
    request_node_t *node = (request_node_t *)malloc(sizeof(request_node_t));
    node->thread_id = thread_id;
    node->request = request;
    node->next = NULL;
    
    if (pending_requests.tail == NULL) {
        pending_requests.head = node;
        pending_requests.tail = node;
    } else {
        pending_requests.tail->next = node;
        pending_requests.tail = node;
    }
}

bool process_pending_requests() {
    request_node_t *current = pending_requests.head;
    request_node_t *prev = NULL;
    bool any_granted = false;
    
    while (current != NULL) {
        /* Check if the request can be granted */
        bool can_grant = true;
        
        /* Check resource availability */
        for (int i = 0; i < m; i++) {
            if (current->request[i] > AVAILABLE[i]) {
                can_grant = false;
                printf("\t+++ Insufficient resources to grant request of thread %02d\n", 
                      current->thread_id);
                break;
            }
        }
        
        /* Check safety (if deadlock avoidance is enabled) */
        if (can_grant) {
            can_grant = is_safe_state(current->thread_id, current->request);
        }
        
        if (can_grant) {
            /* Grant the request */
            for (int i = 0; i < m; i++) {
                AVAILABLE[i] -= current->request[i];
                ALLOCATION[current->thread_id][i] += current->request[i];
                NEED[current->thread_id][i] -= current->request[i];
            }
            
            printf("Master thread grants resource requests for thread %02d\n", 
                  current->thread_id);
            
            /* Signal the waiting thread */
            sleep(1);
            pthread_cond_signal(&cv[current->thread_id]);
            
            /* Remove the node from the queue */
            if (prev == NULL) {
                pending_requests.head = current->next;
            } else {
                prev->next = current->next;
            }
            
            if (current == pending_requests.tail) {
                pending_requests.tail = prev;
            }
            
            request_node_t *temp = current;
            current = current->next;
            free(temp->request);
            free(temp);
            
            any_granted = true;
        } else {
            /* Move to next request */
            prev = current;
            current = current->next;
        }
    }
    
    return any_granted;
}

bool can_allocate(int thread_id, int *request) {
    int i;
    
    /* Check if request exceeds available resources */
    for (i = 0; i < m; i++) {
        if (request[i] > AVAILABLE[i]) {
            return false;
        }
    }
    
    /* Check if request exceeds maximum needs */
    for (i = 0; i < m; i++) {
        if (ALLOCATION[thread_id][i] + request[i] > NEED[thread_id][i]) {
            return false;
        }
    }
    
    return true;
}

void handle_release_request(int thread_id, int *request) {
    for (int i = 0; i < m; i++) {
        /* For release requests, values are negative */
        AVAILABLE[i] += -request[i];
        ALLOCATION[thread_id][i] += request[i];
    }
    free(request);
}

void print_waiting_threads() {
    request_node_t *current = pending_requests.head;
    
    printf("\t\tWaiting threads : ");
    
    while (current != NULL) {
        printf("%02d ", current->thread_id);
        current = current->next;
    }
    
    printf("\n");
}

void format_thread_id(int id, char *buffer) {
    sprintf(buffer, "%02d", id);
}

void read_system_configuration() {
    FILE *fp;
    char buffer[100];
    char filepath[100];
    int i;
    
    /* Open system configuration file */
    sprintf(filepath, "%s/system.txt", INPUT_DIR);
    fp = fopen(filepath, "r");
    
    if (!fp) {
        printf("Error: Cannot open file %s\n", filepath);
        exit(1);
    }
    
    /* Read m (number of resource types) */
    fgets(buffer, sizeof(buffer), fp);
    m = atoi(buffer);
    
    /* Read n (number of threads) */
    fgets(buffer, sizeof(buffer), fp);
    n = atoi(buffer);
    
    /* Read available resources */
    fgets(buffer, sizeof(buffer), fp);
    
    /* Allocate memory for resource state arrays */
    AVAILABLE = (int *)malloc(m * sizeof(int));
    ALLOCATION = (int **)malloc(n * sizeof(int *));
    NEED = (int **)malloc(n * sizeof(int *));
    
    for (i = 0; i < n; i++) {
        ALLOCATION[i] = (int *)calloc(m, sizeof(int));
        NEED[i] = (int *)malloc(m * sizeof(int));
    }
    
    /* Parse available resources */
    char *token = strtok(buffer, " ");
    for (i = 0; i < m && token != NULL; i++) {
        AVAILABLE[i] = atoi(token);
        token = strtok(NULL, " ");
    }
    
    fclose(fp);
}

void read_thread_max_needs(int thread_id, int *needs) {
    FILE *fp;
    char buffer[100];
    char filepath[100];
    int i;
    
    /* Open thread file */
    sprintf(filepath, "%s/thread%02d.txt", INPUT_DIR, thread_id);
    fp = fopen(filepath, "r");
    
    if (!fp) {
        printf("Error: Cannot open file %s\n", filepath);
        exit(1);
    }
    
    /* Read maximum needs */
    fgets(buffer, sizeof(buffer), fp);
    
    /* Parse maximum needs */
    char *token = strtok(buffer, " ");
    for (i = 0; i < m && token != NULL; i++) {
        needs[i] = atoi(token);
        NEED[thread_id][i] = needs[i];
    }
    
    fclose(fp);
}

int get_request_type(int *request) {
    int i;
    
    /* Check if any request component is positive */
    for (i = 0; i < m; i++) {
        if (request[i] > 0) {
            return REQUEST_ADDITIONAL;
        }
    }
    
    /* If all components are non-positive, it's a release request */
    return REQUEST_RELEASE;
}