#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/types.h>

#define MAX_PATH_LEN 4096
#define MAX_USERS 1000

// Structure to store UID to login ID mapping
typedef struct {
    uid_t uid;
    char login[256];
} UserInfo;

// Global variables
UserInfo users[MAX_USERS];
int user_count = 0;
int file_count = 0;
FILE *output_file = NULL;

// Function to load user information from /etc/passwd
void load_user_info() {
    FILE *passwd_file = fopen("/etc/passwd", "r");
    if (!passwd_file) {
        perror("Error opening /etc/passwd");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    while (fgets(line, sizeof(line), passwd_file) && user_count < MAX_USERS) {
        char *token = strtok(line, ":");
        if (!token) continue; // Skip invalid lines
        
        char username[256];
        strncpy(username, token, sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
        
        // Skip to the UID field (third field)
        token = strtok(NULL, ":"); // Skip password field
        token = strtok(NULL, ":");
        if (!token) continue;
        
        uid_t uid = atoi(token);
        
        // Store in our array
        users[user_count].uid = uid;
        strcpy(users[user_count].login, username);
        user_count++;
    }

    fclose(passwd_file);
}

// Function to get login ID from UID
const char* get_login_from_uid(uid_t uid) {
    // First check our cached user info
    for (int i = 0; i < user_count; i++) {
        if (users[i].uid == uid) {
            return users[i].login;
        }
    }
    
    // If not found in our cache, try to get it from the system
    struct passwd *pwd = getpwuid(uid);
    if (pwd) {
        // Add to our cache if there's space
        if (user_count < MAX_USERS) {
            users[user_count].uid = uid;
            strcpy(users[user_count].login, pwd->pw_name);
            user_count++;
        }
        return pwd->pw_name;
    }
    
    // Return UID as string if login not found
    static char uid_str[32];
    sprintf(uid_str, "%u", uid);
    return uid_str;
}

// Function to check if a file has the specified extension
int has_extension(const char *filename, const char *extension) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return 0;
    return strcmp(dot + 1, extension) == 0;
}

// Recursive function to search for files with specified extension
void search_directory(const char *dir_path, const char *extension) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error opening directory %s: %s\n", dir_path, strerror(errno));
        return;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Create full path
        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        // Get file status
        struct stat st;
        if (lstat(full_path, &st) == -1) {
            fprintf(stderr, "Error getting status for %s: %s\n", full_path, strerror(errno));
            continue;
        }
        
        // If it's a regular file with the specified extension
        if (S_ISREG(st.st_mode) && has_extension(entry->d_name, extension)) {
            // Get the owner login ID
            const char *owner = get_login_from_uid(st.st_uid);
            
            // Print the information to file
            fprintf(output_file, "%-3d : %-8s %-8ld %s\n", ++file_count, owner, (long)st.st_size, full_path);
            
            // Also print to console for tracking progress
            printf("%-3d : %-8s %-8ld %s\n", file_count, owner, (long)st.st_size, full_path);
        }
        
        // If it's a directory, search it recursively
        if (S_ISDIR(st.st_mode)) {
            search_directory(full_path, extension);
        }
    }
    
    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <extension>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *directory = argv[1];
    const char *extension = argv[2];
    
    // Create output filename: finall_<extension>.txt
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "finall_%s.txt", extension);
    
    // Open output file
    output_file = fopen(output_filename, "w");
    if (!output_file) {
        perror("Error opening output file");
        return EXIT_FAILURE;
    }
    
    // Load user information
    load_user_info();
    
    // Print header to both console and file
    printf("NO  : OWNER\t SIZE\t\t NAME\n");
    printf("--    -----\t ----\t\t ----\n");
    fprintf(output_file, "NO  : OWNER\t SIZE\t\t NAME\n");
    fprintf(output_file, "--    -----\t ----\t\t ----\n");
    
    // Start search
    search_directory(directory, extension);
    
    // Print footer to both console and file
    printf("+++ %d files match the extension %s\n", file_count, extension);
    fprintf(output_file, "+++ %d files match the extension %s\n", file_count, extension);
    
    // Close output file
    fclose(output_file);
    
    printf("Results saved to %s\n", output_filename);
    
    return EXIT_SUCCESS;
}