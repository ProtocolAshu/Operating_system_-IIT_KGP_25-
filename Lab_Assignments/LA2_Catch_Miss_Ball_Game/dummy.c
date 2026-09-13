#include <signal.h>
#include <stdio.h>
#include <unistd.h>

int main() {
    while (1) {
        pause();  // Infinite loop waiting for signals
    }
    return 0;
}