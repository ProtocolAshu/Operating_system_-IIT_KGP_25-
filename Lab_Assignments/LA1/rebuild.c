#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_FOODULES 100
#define DONE_FILE "done.txt"
#define DEP_FILE "foodep.txt"

// Function to initialize the "done" array to all zeros
void initialize_done(int n) {
    FILE *done_file = fopen(DONE_FILE, "w");
    if (!done_file) {
        perror("Error opening done.txt");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        fprintf(done_file, "0\n");
    }
    fclose(done_file);
}

// Function to read the "done" array
void read_done(int *done, int n) {
    FILE *done_file = fopen(DONE_FILE, "r");
    if (!done_file) {
        perror("Error reading done.txt");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        fscanf(done_file, "%d", &done[i]);
    }
    fclose(done_file);
}

// Function to update the "done" array
void update_done(int foodule, int n) {
    int done[MAX_FOODULES];
    FILE *done_file = fopen(DONE_FILE, "r+");
    if (!done_file) {
        perror("Error opening done.txt for update");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        if (fscanf(done_file, "%d", &done[i]) != 1) {
            break;
        }
    }

    done[foodule - 1] = 1; // Mark as rebuilt

    fseek(done_file, 0, SEEK_SET);
    for (int i = 0; i < n; i++) {
        fprintf(done_file, "%d\n", done[i]);
    }

    fclose(done_file);
}

// Function to process a single foodule
void process_foodule(int foodule, int n) {
    FILE *dep_file = fopen(DEP_FILE, "r");
    if (!dep_file) {
        perror("Error reading foodep.txt");
        exit(EXIT_FAILURE);
    }

    int done[MAX_FOODULES];
    read_done(done, n);

    // Skip the first line (number of foodules)
    int total_foodules;
    fscanf(dep_file, "%d", &total_foodules);

    char line[256];
    char rebuilt_from[256] = ""; // To store dependencies
    int first_dependency = 1;

    while (fgets(line, sizeof(line), dep_file)) {
        int u;
        if (sscanf(line, "%d:", &u) == 1 && u == foodule) {
            char *token = strtok(line, ": ");
            while ((token = strtok(NULL, " \n")) != NULL) {
                int dependency = atoi(token);
                if (done[dependency - 1] == 0) {
                    pid_t pid = fork();
                    if (pid == 0) {
                        // Child process
                        char foodule_str[10];
                        snprintf(foodule_str, sizeof(foodule_str), "%d", dependency);
                        execlp("./rebuild", "rebuild", foodule_str, "child", NULL);
                        perror("Error executing rebuild");
                        exit(EXIT_FAILURE);
                    } else if (pid > 0) {
                        // Parent process
                        wait(NULL);
                        read_done(done, n); // Refresh the done array
                        if (!first_dependency) {
                            strcat(rebuilt_from, ", ");
                        }
                        first_dependency = 0;
                        char temp[10];
                        snprintf(temp, sizeof(temp), "foo%d", dependency);
                        strcat(rebuilt_from, temp);
                    } else {
                        perror("Error forking");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    // Dependency already rebuilt, include it in the output
                    if (!first_dependency) {
                        strcat(rebuilt_from, ", ");
                    }
                    first_dependency = 0;
                    char temp[10];
                    snprintf(temp, sizeof(temp), "foo%d", dependency);
                    strcat(rebuilt_from, temp);
                }
            }
            break;
        }
    }

    fclose(dep_file);

    // Mark current foodule as rebuilt
    if (done[foodule - 1] == 0) {
        if (strlen(rebuilt_from) > 0) {
            printf("foo%d rebuilt from %s\n", foodule, rebuilt_from);
        } else {
            printf("foo%d rebuilt\n", foodule);
        }
        update_done(foodule, n);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <foodule> [child]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int foodule = atoi(argv[1]);

    FILE *dep_file = fopen(DEP_FILE, "r");
    if (!dep_file) {
        perror("Error opening foodep.txt");
        exit(EXIT_FAILURE);
    }

    int n;
    fscanf(dep_file, "%d", &n);
    fclose(dep_file);

    if (argc == 2) {
        initialize_done(n);
    }

    process_foodule(foodule, n);

    return 0;
}
