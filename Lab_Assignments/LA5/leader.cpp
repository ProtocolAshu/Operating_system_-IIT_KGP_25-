#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>

class Leader {
public:  // Public constants
    static const int DEFAULT_N = 10;
    static const int MAX_FOLLOWERS = 100;

private:
    static const int HASH_SIZE = 1000;
    static const int TERMINATION_SIGNAL = -1;

    struct HashNode {
        int sum;
        bool isOccupied;
        HashNode() : sum(0), isOccupied(false) {}
    };

    int n;
    int shmid;
    int* M;
    HashNode* hashTable;

public:
    Leader(int num_followers = DEFAULT_N) : n(num_followers), shmid(-1), M(nullptr), hashTable(nullptr) {
        if (n > MAX_FOLLOWERS) {
            throw std::runtime_error("Error: Maximum number of followers is " + std::to_string(MAX_FOLLOWERS));
        }
    }

    ~Leader() {
        cleanup();
    }

    void run() {
        initialize_shared_memory();
        initialize_hash_table();
        wait_for_followers();
        main_loop();
    }

private:
    void initialize_shared_memory() {
        key_t key = ftok("/", 65);
        int shm_size = (4 + n) * sizeof(int);
        shmid = shmget(key, shm_size, IPC_CREAT | IPC_EXCL | 0666);
        
        if (shmid == -1) {
            throw std::runtime_error("Error: Another leader is already running");
        }

        M = static_cast<int*>(shmat(shmid, nullptr, 0));
        if (M == reinterpret_cast<int*>(-1)) {
            throw std::runtime_error("Error: Cannot attach shared memory");
        }

        M[0] = n;      // Number of followers
        M[1] = 0;      // Number of followers joined
        M[2] = 0;      // Turn (0 for leader)
    }

    void initialize_hash_table() {
        hashTable = new HashNode[HASH_SIZE]();
        std::srand(static_cast<unsigned>(std::time(nullptr)));
    }

    void wait_for_followers() {
        // std::cout << "Wait for the moment." << std::endl;
        while (M[1] < n) {
            usleep(1000);
        }
    }

    void main_loop() {
        while (true) {
            while (M[2] != 0) {
                usleep(1000);
            }

            M[3] = std::rand() % 99 + 1;
            M[2] = 1;  // Give turn to first follower

            while (M[2] != 0) {
                usleep(1000);
            }

            int sum = calculate_and_print_sum();
            if (check_and_update_hash(sum)) {
                break;
            }
        }
    }

    int calculate_and_print_sum() {
        int sum = 0;
        std::cout << M[3];
        
        for (int i = 3; i <= 3 + n; i++) {
            if (i > 3) {
                std::cout << " + " << M[i];
            }
            sum += M[i];
        }
        std::cout << " = " << sum << std::endl;
        return sum;
    }

    bool check_and_update_hash(int sum) {
        int hash_index = sum % HASH_SIZE;
        while (hashTable[hash_index].isOccupied) {
            if (hashTable[hash_index].sum == sum) {
                M[2] = TERMINATION_SIGNAL;
                while (M[2] != 0) {
                    usleep(1000);
                }
                return true;
            }
            hash_index = (hash_index + 1) % HASH_SIZE;
        }

        hashTable[hash_index].sum = sum;
        hashTable[hash_index].isOccupied = true;
        return false;
    }

    void cleanup() {
        delete[] hashTable;
        if (M != nullptr) {
            shmdt(M);
        }
        if (shmid != -1) {
            shmctl(shmid, IPC_RMID, nullptr);
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        int n = (argc > 1) ? std::atoi(argv[1]) : Leader::DEFAULT_N;
        Leader leader(n);
        leader.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}