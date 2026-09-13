#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include "boardgen.c"

#define BLOCK_COUNT 9
#define BLOCK_SIZE 3

// Structure to store pipe file descriptors
typedef struct {
    int read_fd;
    int write_fd;
} PipePair;

// Get row neighbors for a block
void get_row_neighbors(int block, int* n1, int* n2) {
    int row = block / 3;
    int base = row * 3;
    *n1 = (block + 1) % 3 + base;
    *n2 = (block + 2) % 3 + base;
}

// Get column neighbors for a block
void get_column_neighbors(int block, int* n1, int* n2) {
    // Calculate column-based neighbors
    *n1 = (block + 3) % 9;
    if (*n1 / 3 == block / 3) *n1 = (block + 6) % 9;
    *n2 = (block + 6) % 9;
    if (*n2 / 3 == block / 3) *n2 = (block + 3) % 9;
}

// Launch xterm with block process
void launch_block(int block_num, PipePair pipes[], int x, int y) {
    int row_n1, row_n2, col_n1, col_n2;
    get_row_neighbors(block_num, &row_n1, &row_n2);
    get_column_neighbors(block_num, &col_n1, &col_n2);
    
    char geometry[32];
    snprintf(geometry, sizeof(geometry), "17x8+%d+%d", x, y);
    
    char block_title[32];
    snprintf(block_title, sizeof(block_title), "Block %d", block_num);
    
    char bno[8], rfd[8], wfd[8];
    char rn1[8], rn2[8], cn1[8], cn2[8];
    
    snprintf(bno, sizeof(bno), "%d", block_num);
    snprintf(rfd, sizeof(rfd), "%d", pipes[block_num].read_fd);
    snprintf(wfd, sizeof(wfd), "%d", pipes[block_num].write_fd);
    snprintf(rn1, sizeof(rn1), "%d", pipes[row_n1].write_fd);
    snprintf(rn2, sizeof(rn2), "%d", pipes[row_n2].write_fd);
    snprintf(cn1, sizeof(cn1), "%d", pipes[col_n1].write_fd);
    snprintf(cn2, sizeof(cn2), "%d", pipes[col_n2].write_fd);
    
    execlp("xterm", "xterm", 
           "-T", block_title,
           "-fa", "Monospace",
           "-fs", "15",
           "-geometry", geometry,
           "-bg", "#331100",
           "-fg", "white",
           "-e", "./block",
           bno, rfd, wfd, rn1, rn2, cn1, cn2,
           NULL);
    
    perror("execlp failed");
    exit(1);
}

// Print help message
void print_help() {
    printf("\nFoodoku Commands:\n");
    printf("h - Show this help message\n");
    printf("n - Start new game\n");
    printf("p b c d - Place digit d in cell c of block b\n");
    printf("s - Show solution\n");
    printf("q - Quit game\n\n");
    printf("Blocks are numbered 0-8 from left to right, top to bottom\n");
    printf("Cells within blocks are numbered 0-8 similarly\n\n");
}

int main() {
    PipePair pipes[BLOCK_COUNT];
    pid_t children[BLOCK_COUNT];
    int A[9][9], S[9][9];
    char command;
    
    // Create pipes
    for (int i = 0; i < BLOCK_COUNT; i++) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe creation failed");
            exit(1);
        }
        pipes[i].read_fd = pipefd[0];
        pipes[i].write_fd = pipefd[1];
    }
    
    // Fork children and launch xterms
    for (int i = 0; i < BLOCK_COUNT; i++) {
        children[i] = fork();
        if (children[i] == -1) {
            perror("fork failed");
            exit(1);
        }
        
        if (children[i] == 0) {
            // Child process
            int row_n1, row_n2, col_n1, col_n2;
            get_row_neighbors(i, &row_n1, &row_n2);
            get_column_neighbors(i, &col_n1, &col_n2);
            
            // Close unused pipe ends
            for (int j = 0; j < BLOCK_COUNT; j++) {
                if (j != i) {
                    close(pipes[j].read_fd);
                    if (j != row_n1 && j != row_n2 && j != col_n1 && j != col_n2) {
                        close(pipes[j].write_fd);
                    }
                }
            }
            
            // Calculate xterm position (300px spacing)
            int x = 800 + (i % 3) * 300;
            int y = 300 + (i / 3) * 300;
            
            launch_block(i, pipes, x, y);
            // Should never reach here due to exec
            exit(1);
        }
    }
    
    // Parent process
    // Close read ends of all pipes
    for (int i = 0; i < BLOCK_COUNT; i++) {
        close(pipes[i].read_fd);
    }
    
    // Set up signal handling for clean exit
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    
    print_help();
    
    while (1) {
        printf("Enter command: ");
        fflush(stdout);
        
        if (scanf(" %c", &command) != 1) {
            printf("Error reading command\n");
            continue;
        }
        
        switch (command) {
            case 'h':
                print_help();
                break;
                
            case 'n':
                newboard(A, S);
                // Send initial board state to each block
                for (int i = 0; i < BLOCK_COUNT; i++) {
                    dprintf(pipes[i].write_fd, "n ");
                    int block_row = (i / 3) * 3;
                    int block_col = (i % 3) * 3;
                    for (int r = 0; r < 3; r++) {
                        for (int c = 0; c < 3; c++) {
                            dprintf(pipes[i].write_fd, "%d ", 
                                    A[block_row + r][block_col + c]);
                        }
                    }
                    dprintf(pipes[i].write_fd, "\n");
                }
                printf("New game started!\n");
                break;
                
            case 'p': {
                int b, c, d;
                if (scanf("%d %d %d", &b, &c, &d) != 3) {
                    printf("Invalid input format. Use: p block_num cell_num digit\n");
                    break;
                }
                if (b < 0 || b >= 9 || c < 0 || c >= 9 || d < 1 || d > 9) {
                    printf("Invalid input range\n");
                    printf("block_num and cell_num must be between 0 and 8\n");
                    printf("digit must be between 1 and 9\n");
                    break;
                }
                dprintf(pipes[b].write_fd, "p %d %d\n", c, d);
                break;
            }
                
            case 's':
                // Send solution to each block
                for (int i = 0; i < BLOCK_COUNT; i++) {
                    dprintf(pipes[i].write_fd, "n ");
                    int block_row = (i / 3) * 3;
                    int block_col = (i % 3) * 3;
                    for (int r = 0; r < 3; r++) {
                        for (int c = 0; c < 3; c++) {
                            dprintf(pipes[i].write_fd, "%d ", 
                                    S[block_row + r][block_col + c]);
                        }
                    }
                    dprintf(pipes[i].write_fd, "\n");
                }
                printf("Solution displayed\n");
                break;
                
            case 'q':
                printf("\nExiting game...\n");
                // Send quit command to all blocks
                for (int i = 0; i < BLOCK_COUNT; i++) {
                    dprintf(pipes[i].write_fd, "q\n");
                }
                
                // Wait for all children to exit
                for (int i = 0; i < BLOCK_COUNT; i++) {
                    wait(NULL);
                }
                
                // Close all write pipe ends
                for (int i = 0; i < BLOCK_COUNT; i++) {
                    close(pipes[i].write_fd);
                }
                
                printf("Game over. Thanks for playing!\n");
                exit(0);
                break;
                
            default:
                printf("Invalid command. Type 'h' for help.\n");
        }
    }
    
    return 0;
}