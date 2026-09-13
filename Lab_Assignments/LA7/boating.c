#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

// Structure for counting semaphore
typedef struct {
    int value;
    pthread_mutex_t mtx;
    pthread_cond_t cv;
} semaphore;

// Function prototypes
void P(semaphore *s);
void V(semaphore *s);
void *boat_function(void *arg);
void *visitor_function(void *arg);

// Global variables
int m, n;                      // m boats, n visitors
int completed_visitors = 0;    // Count of visitors who have completed rides
int last_boat_id = -1;         // ID of the last boat to carry a visitor

semaphore boat;                // Semaphore for boats 
semaphore rider;               // Semaphore for riders
pthread_mutex_t bmtx;          // Mutex for boat-rider matching
pthread_mutex_t count_mtx;     // Mutex for updating completed_visitors

bool *BA;                      // Boat available array
int *BC;                       // Boat-visitor connection array
int *BT;                       // Boat ride time array
pthread_barrier_t *BB;         // Array of barriers (one per boat)
pthread_barrier_t EOS;         // End of session barrier

// P operation (wait) on semaphore
void P(semaphore *s) {
    pthread_mutex_lock(&s->mtx);
    s->value--;
    if (s->value < 0) {
        pthread_cond_wait(&s->cv, &s->mtx);
    }
    pthread_mutex_unlock(&s->mtx);
}

// V operation (signal) on semaphore
void V(semaphore *s) {
    pthread_mutex_lock(&s->mtx);
    s->value++;
    if (s->value <= 0) {
        pthread_cond_signal(&s->cv);
    }
    pthread_mutex_unlock(&s->mtx);
}

// Boat thread function
void *boat_function(void *arg) {
    int id = *((int *)arg);
    free(arg);
    
    printf("Boat %d Ready\n", id);
    
    // Initialize barrier for this boat
    pthread_barrier_init(&BB[id-1], NULL, 2);
    
    while (1) {
        // Signal that a boat is available
        V(&rider);
        
        // Wait for a visitor
        P(&boat);
        
        // Lock the mutex to update shared data
        pthread_mutex_lock(&bmtx);
        BA[id-1] = true;     // Mark this boat as available
        BC[id-1] = -1;       // No visitor assigned yet
        pthread_mutex_unlock(&bmtx);
        
        // Wait for visitor to join
        pthread_barrier_wait(&BB[id-1]);
        
        // Lock mutex to read shared data
        pthread_mutex_lock(&bmtx);
        BA[id-1] = false;    // Mark boat as unavailable
        int visitor_id = BC[id-1];
        int ride_time = BT[id-1];
        pthread_mutex_unlock(&bmtx);
        
        if (visitor_id != -1) {
            printf("Boat %d Start of ride for visitor %d\n", id, visitor_id);
            
            // Ride with visitor (scaled down to 100ms per minute)
            usleep(ride_time * 100000);
            
            printf("Boat %d End of ride for visitor %d (ride time = %d)\n", id, visitor_id, ride_time);
            
            // Update completed visitors count
            pthread_mutex_lock(&count_mtx);
            completed_visitors++;
            
            // Check if all visitors have completed their rides
            if (completed_visitors == n) {
                last_boat_id = id;
            }
            pthread_mutex_unlock(&count_mtx);
            
            // If this was the last visitor, exit the loop
            if (completed_visitors == n) {
                break;
            }
        }
    }
    
    // If this is the last boat to carry a visitor, wait on EOS barrier
    if (id == last_boat_id) {
        pthread_barrier_wait(&EOS);
    }
    
    // Clean up barrier for this boat
    pthread_barrier_destroy(&BB[id-1]);
    
    return NULL;
}

// Visitor thread function
void *visitor_function(void *arg) {
    int id = *((int *)arg);
    free(arg);
    
    // Decide random visit time (30-120 minutes) and ride time (15-60 minutes)
    int visit_time = 30 + rand() % 91;  // 30 to 120 minutes
    int ride_time = 15 + rand() % 46;   // 15 to 60 minutes
    
    printf("Visitor %d Starts sightseeing for %d minutes\n", id, visit_time);
    
    // Visit other attractions (scaled down to 100ms per minute)
    usleep(visit_time * 100000);
    
    printf("Visitor %d Ready to ride a boat (ride time = %d)\n", id, ride_time);
    
    // Signal boat that visitor is ready
    V(&boat);
    
    // Wait for a boat to be available
    P(&rider);
    
    // Find an available boat
    int boat_id = -1;
    bool found_boat = false;
    
    while (!found_boat) {
        pthread_mutex_lock(&bmtx);
        
        // Search for an available boat
        for (int i = 0; i < m; i++) {
            if (BA[i] && BC[i] == -1) {
                boat_id = i + 1;
                BC[i] = id;
                BT[i] = ride_time;
                found_boat = true;
                break;
            }
        }
        
        pthread_mutex_unlock(&bmtx);
        
        if (!found_boat) {
            // If no boat found, sleep a little before trying again
            usleep(1000); // 1ms sleep to avoid busy waiting
        }
    }
    
    printf("Visitor %d Finds boat %d\n", id, boat_id);
    
    // Join the barrier with the boat to start the ride
    pthread_barrier_wait(&BB[boat_id-1]);
    
    // After ride is complete, the visitor leaves
    printf("Visitor %d Leaving\n", id);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Validate command line arguments
    if (argc != 3) {
        printf("Usage: %s <number_of_boats> <number_of_visitors>\n", argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    m = atoi(argv[1]);
    n = atoi(argv[2]);
    
    // Validate input values
    if (m < 5 || m > 10 || n < 20 || n > 100) {
        printf("Invalid input. Boats should be 5-10 and visitors should be 20-100.\n");
        return 1;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Initialize semaphores
    boat = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    rider = (semaphore){0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER};
    
    // Initialize mutexes
    pthread_mutex_init(&bmtx, NULL);
    pthread_mutex_init(&count_mtx, NULL);
    
    // Initialize barrier EOS to 2 (main thread and last boat)
    pthread_barrier_init(&EOS, NULL, 2);
    
    // Allocate memory for arrays
    BA = (bool *)malloc(m * sizeof(bool));
    BC = (int *)malloc(m * sizeof(int));
    BT = (int *)malloc(m * sizeof(int));
    BB = (pthread_barrier_t *)malloc(m * sizeof(pthread_barrier_t));
    
    // Check if memory allocation was successful
    if (!BA || !BC || !BT || !BB) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Initialize arrays
    for (int i = 0; i < m; i++) {
        BA[i] = false;
        BC[i] = -1;
        BT[i] = 0;
    }
    
    // Create threads
    pthread_t *boat_threads = (pthread_t *)malloc(m * sizeof(pthread_t));
    pthread_t *visitor_threads = (pthread_t *)malloc(n * sizeof(pthread_t));
    
    // Check if memory allocation was successful
    if (!boat_threads || !visitor_threads) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Start boat threads
    for (int i = 0; i < m; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (!id) {
            printf("Memory allocation failed\n");
            return 1;
        }
        *id = i + 1;
        if (pthread_create(&boat_threads[i], NULL, boat_function, (void *)id) != 0) {
            printf("Failed to create boat thread %d\n", i+1);
            return 1;
        }
    }
    
    // Start visitor threads
    for (int i = 0; i < n; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (!id) {
            printf("Memory allocation failed\n");
            return 1;
        }
        *id = i + 1;
        if (pthread_create(&visitor_threads[i], NULL, visitor_function, (void *)id) != 0) {
            printf("Failed to create visitor thread %d\n", i+1);
            return 1;
        }
    }
    
    // Wait for the last boat to finish on the EOS barrier
    pthread_barrier_wait(&EOS);
    printf("All visitors have completed their boat rides. Simulation ending.\n");
    
    // Join all threads
    for (int i = 0; i < m; i++) {
        pthread_join(boat_threads[i], NULL);
    }
    
    for (int i = 0; i < n; i++) {
        pthread_join(visitor_threads[i], NULL);
    }
    
    // Clean up resources
    pthread_mutex_destroy(&bmtx);
    pthread_mutex_destroy(&count_mtx);
    pthread_barrier_destroy(&EOS);
    pthread_mutex_destroy(&boat.mtx);
    pthread_mutex_destroy(&rider.mtx);
    pthread_cond_destroy(&boat.cv);
    pthread_cond_destroy(&rider.cv);
    
    free(BA);
    free(BC);
    free(BT);
    free(BB);
    free(boat_threads);
    free(visitor_threads);
    
    return 0;
}