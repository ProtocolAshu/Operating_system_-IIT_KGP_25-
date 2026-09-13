#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <vector>

class Follower {
private:
    int follower_num;
    int* M;

public:
    Follower(int num, int* shared_mem) : follower_num(num), M(shared_mem) {
        std::srand(static_cast<unsigned>(std::time(nullptr)) + follower_num);
    }

    void run() {
        std::cout << "follower " << follower_num << " joins" << std::endl;

        while (true) {
            // Wait for my turn
            while (M[2] != follower_num && M[2] != -follower_num) {
                usleep(1000);
            }

            // Check if it's time to terminate
            if (M[2] == -follower_num) {
                std::cout << "follower " << follower_num << " leaves" << std::endl;
                // Set turn for next follower or leader
                M[2] = (follower_num == M[0]) ? 0 : -(follower_num + 1);
                return;
            }

            // Generate random number and write to M[3 + follower_num]
            M[3 + follower_num] = std::rand() % 9 + 1;

            // Give turn to next follower or back to leader
            M[2] = (follower_num == M[0]) ? 0 : follower_num + 1;
        }
    }
};

class FollowerManager {
public:
    static const int DEFAULT_NF = 1;
    
private:
    int nf;
    int* M;
    int shmid;
    std::vector<pid_t> children;

public:
    FollowerManager(int num_followers = DEFAULT_NF) : nf(num_followers), M(nullptr), shmid(-1) {
        if (nf < 1) {
            throw std::runtime_error("Error: Number of followers must be positive");
        }
    }

    ~FollowerManager() {
        if (M != nullptr) {
            shmdt(M);
        }
    }

    void run() {
        attach_shared_memory();
        create_followers();
        wait_for_children();
    }

private:
    void attach_shared_memory() {
        key_t key = ftok("/", 65);
        shmid = shmget(key, 0, 0666);
        
        if (shmid == -1) {
            throw std::runtime_error("Error: Leader is not running");
        }

        M = static_cast<int*>(shmat(shmid, nullptr, 0));
        if (M == reinterpret_cast<int*>(-1)) {
            throw std::runtime_error("Error: Cannot attach shared memory");
        }
    }

    void create_followers() {
        for (int i = 0; i < nf; i++) {
            pid_t pid = fork();
            
            if (pid == 0) {  // Child process
                int follower_num = ++M[1];
                
                if (follower_num > M[0]) {
                    std::cout << "follower error: " << M[0] << " followers have already joined" << std::endl;
                    exit(1);
                }

                Follower follower(follower_num, M);
                follower.run();
                exit(0);
            }
            else if (pid > 0) {
                children.push_back(pid);
            }
            else {
                throw std::runtime_error("Error: Fork failed");
            }
        }
    }

    void wait_for_children() {
        for (pid_t pid : children) {
            waitpid(pid, nullptr, 0);
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        int nf = (argc > 1) ? std::atoi(argv[1]) : FollowerManager::DEFAULT_NF;
        FollowerManager manager(nf);
        manager.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}