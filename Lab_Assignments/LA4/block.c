#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BLOCK_SIZE 3

typedef struct {
    int A[BLOCK_SIZE][BLOCK_SIZE];  // Original puzzle block
    int B[BLOCK_SIZE][BLOCK_SIZE];  // Current state
    int block_num;                  // Block number (0-8)
} BlockState;

void draw_block(BlockState* state) {
    printf("\033[H\033[J");  // Clear screen
    printf("Block %d\n", state->block_num);
    printf("┌───┬───┬───┐\n");
    for (int i = 0; i < BLOCK_SIZE; i++) {
        printf("│");
        for (int j = 0; j < BLOCK_SIZE; j++) {
            if (state->B[i][j] == 0)
                printf(" . ");
            else
                printf(" %d ", state->B[i][j]);
            printf("│");
        }
        printf("\n");
        if (i < BLOCK_SIZE - 1)
            printf("├───┼───┼───┤\n");
    }
    printf("└───┴───┴───┘\n");
    fflush(stdout);
}

void print_error(const char* msg) {
    printf("\n%s\n", msg);
    fflush(stdout);
    sleep(2);  // Wait for 2 seconds
}

int check_conflict(int array[BLOCK_SIZE], int digit) {
    for (int i = 0; i < BLOCK_SIZE; i++) {
        if (array[i] == digit) return 1;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 8) {
        fprintf(stderr, "Usage: %s block_num read_fd write_fd row_n1_fd row_n2_fd col_n1_fd col_n2_fd\n", 
                argv[0]);
        exit(1);
    }
    
    BlockState state;
    state.block_num = atoi(argv[1]);
    int read_fd = atoi(argv[2]);
    int write_fd = atoi(argv[3]);
    int row_n1_fd = atoi(argv[4]);
    int row_n2_fd = atoi(argv[5]);
    int col_n1_fd = atoi(argv[6]);
    int col_n2_fd = atoi(argv[7]);
    
    // Redirect stdin to read from pipe
    if (dup2(read_fd, STDIN_FILENO) == -1) {
        perror("dup2 failed");
        exit(1);
    }
    close(read_fd);
    
    char command;
    
    while (scanf(" %c", &command) == 1) {
        switch (command) {
            case 'n': {
                // Receive new board state
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    for (int j = 0; j < BLOCK_SIZE; j++) {
                        scanf("%d", &state.A[i][j]);
                        state.B[i][j] = state.A[i][j];
                    }
                }
                draw_block(&state);
                break;
            }
                
            case 'p': {
                int cell, digit;
                scanf("%d %d", &cell, &digit);
                
                int row = cell / 3;
                int col = cell % 3;
                
                // Check if trying to modify original puzzle
                if (state.A[row][col] != 0) {
                    print_error("Read-only cell");
                    draw_block(&state);
                    break;
                }
                
                // Check block conflict
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    for (int j = 0; j < BLOCK_SIZE; j++) {
                        if (state.B[i][j] == digit) {
                            print_error("Block conflict");
                            draw_block(&state);
                            goto end_placement;
                        }
                    }
                }
                
                // Check row conflicts
                dprintf(row_n1_fd, "r %d %d %d\n", row, digit, write_fd);
                dprintf(row_n2_fd, "r %d %d %d\n", row, digit, write_fd);
                
                int response;
                scanf("%d", &response);
                if (response != 0) {
                    print_error("Row conflict");
                    draw_block(&state);
                    break;
                }
                scanf("%d", &response);
                if (response != 0) {
                    print_error("Row conflict");
                    draw_block(&state);
                    break;
                }
                
                // Check column conflicts
                dprintf(col_n1_fd, "c %d %d %d\n", col, digit, write_fd);
                dprintf(col_n2_fd, "c %d %d %d\n", col, digit, write_fd);
                
                scanf("%d", &response);
                if (response != 0) {
                    print_error("Column conflict");
                    draw_block(&state);
                    break;
                }
                scanf("%d", &response);
                if (response != 0) {
                    print_error("Column conflict");
                    draw_block(&state);
                    break;
                }
                
                // Place digit
                state.B[row][col] = digit;
                draw_block(&state);
                
            end_placement:
                break;
            }

                
            case 'r': {
                int row, digit, reply_fd;
                scanf("%d %d %d", &row, &digit, &reply_fd);
                
                int row_values[BLOCK_SIZE];
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    row_values[j] = state.B[row][j];
                }
                
                dprintf(reply_fd, "%d\n", check_conflict(row_values, digit));
                break;
            }
                
            case 'c': {
                int col, digit, reply_fd;
                scanf("%d %d %d", &col, &digit, &reply_fd);
                
                int col_values[BLOCK_SIZE];
                for (int i = 0; i < BLOCK_SIZE; i++) {
                    col_values[i] = state.B[i][col];
                }
                
                dprintf(reply_fd, "%d\n", check_conflict(col_values, digit));
                break;
            }
                
            case 'q':
                print_error("Bye...");
                exit(0);
                break;

    
        }
    }
    
    return 0;
}