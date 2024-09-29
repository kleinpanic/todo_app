#ifndef TODO_H
#define TODO_H

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Define necessary constants
#define MAX_TASKS 100
#define MAX_TITLE_LEN 256
#define MAX_CATEGORY_LEN 50
#define MAX_DATE_LEN 12  // Adjusted to fit YYYY-MM-DD format with null terminator
#define MAX_RECURRENCE_LEN 10  // Define MAX_RECURRENCE_LEN for recurrence string length
#define LOCAL_FILE_PATH ".local/share/todo/tasks.txt"
#define NO_DUE_DATE "N/A"  // Custom marker for no due date

// Function to get the database path, either global or local
static char *get_database_path() {
    static char file_path[512];  // To store the resolved path

    // Check if the program is running from a global location by checking if /usr/local/bin/todo exists
    if (access("/usr/local/bin/todo", F_OK) == 0) {
        // Use global path, ensure ~/.local/share/todo/tasks.txt exists
        char *home = getenv("HOME");
        snprintf(file_path, sizeof(file_path), "%s/%s", home, LOCAL_FILE_PATH);

        // Ensure the directory exists, create it if not
        char dir_path[512];
        snprintf(dir_path, sizeof(dir_path), "%s/.local/share/todo", home);
        if (access(dir_path, F_OK) != 0) {
            mkdir(dir_path, 0755);  // Create the directory with appropriate permissions
        }

        // Check if the tasks.txt file exists, create if not
        if (access(file_path, F_OK) != 0) {
            int fd = open(file_path, O_CREAT | O_RDWR, 0644);  // Create file with read/write for owner, read for others
            if (fd != -1) {
                close(fd);  // Close the file after creating it
            }
        }
    } else {
        // Local path for users without global installation
        snprintf(file_path, sizeof(file_path), "%s/%s", getenv("HOME"), LOCAL_FILE_PATH);
        
        // Ensure the directory exists, create it if not
        char dir_path[512];
        snprintf(dir_path, sizeof(dir_path), "%s/.local/share/todo", getenv("HOME"));
        if (access(dir_path, F_OK) != 0) {
            mkdir(dir_path, 0755);  // Create directory if it doesn't exist
        }

        // Create tasks.txt file if it doesn't exist
        if (access(file_path, F_OK) != 0) {
            int fd = open(file_path, O_CREAT | O_RDWR, 0644);  // Create file
            if (fd != -1) {
                close(fd);  // Close file after creating it
            }
        }
    }

    return file_path;
}

// Task structure definition
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char category[MAX_CATEGORY_LEN];
    char due_date[MAX_DATE_LEN];
    char recurrence[MAX_RECURRENCE_LEN];  // Recurrence interval (e.g., "daily", "weekly")
    int priority;
    int completed;
} Task;

// Function prototypes
void add_task(Task tasks[], int *count, const char *title, const char *category, const char *due_date, const char *recurrence, int priority);
void remove_task(Task tasks[], int *count, int index);  // Changed to accept index
void edit_task(Task *task);
void load_tasks(Task tasks[], int *count);
void save_tasks(Task tasks[], int count);
void display_tasks(Task tasks[], int count, int selected);
void search_task(Task tasks[], int count, int *selected_task);

// Sorting function updated to include boolean for ascending/descending order
void sort_tasks(Task tasks[], int count, char sort_type, bool ascending);

// Input handling
void get_input(char *buffer, int size, const char *prompt);
void get_input_and_clear(char *buffer, int size, const char *prompt);

// ncurses management
void init_ncurses();
void cleanup_ncurses();

// Task status manipulation
void toggle_task_completion(Task *task);
void update_task_recurrence(Task *task);

// Utility functions
int is_task_overdue(Task task);
int is_task_due_soon(Task task);

#endif
