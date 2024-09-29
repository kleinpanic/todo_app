#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include "todo.h"

#define MAX_RECURRENCE_LEN 10
#define MAX_DATE_LEN 12  // Adjusted to fit YYYY-MM-DD format with null terminator (12 characters including \0)

// Function to handle getting input from the user
void get_input(char *buffer, int size, const char *prompt) {
    mvprintw(LINES - 2, 0, "%s", prompt);
    echo();
    getnstr(buffer, size - 1);
    noecho();
    buffer[size - 1] = '\0';
    refresh();
}

// Comparator functions for sorting by priority, due date, and completion status
int compare_priority_desc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;
    return taskB->priority - taskA->priority;  // Higher priority first
}

int compare_priority_asc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;
    return taskA->priority - taskB->priority;  // Lower priority first
}

int compare_due_date_desc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;

    if (strcmp(taskA->due_date, "N/A") == 0) return 1;  // Push "N/A" (no due date) to the end
    if (strcmp(taskB->due_date, "N/A") == 0) return -1;

    return strcmp(taskA->due_date, taskB->due_date);  // Closest date first
}

int compare_due_date_asc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;

    if (strcmp(taskA->due_date, "N/A") == 0) return 1;  // Push "N/A" (no due date) to the end
    if (strcmp(taskB->due_date, "N/A") == 0) return -1;

    return strcmp(taskB->due_date, taskA->due_date);  // Latest date first
}

// Function to handle sorting with ascending/descending toggling
void sort_tasks(Task tasks[], int count, char sort_type, bool ascending) {
    switch (sort_type) {
        case 'p':  // Sort by priority
            if (ascending) {
                qsort(tasks, count, sizeof(Task), compare_priority_asc);
            } else {
                qsort(tasks, count, sizeof(Task), compare_priority_desc);
            }
            break;
        case 'd':  // Sort by due date
            if (ascending) {
                qsort(tasks, count, sizeof(Task), compare_due_date_desc);
            } else {
                qsort(tasks, count, sizeof(Task), compare_due_date_asc);
            }
            break;
    }
}

void init_ncurses() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    refresh();
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);     // Red for overdue tasks
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Yellow for due soon tasks
}

void cleanup_ncurses() {
    endwin();
}

void toggle_task_completion(Task *task) {
    // Toggle the completion status (1 for completed, 0 for not completed)
    task->completed = !task->completed;

    // If the task is recurring, update the due date when completed
    if (task->completed && strcmp(task->recurrence, "none") != 0) {
        update_task_recurrence(task);
    }

    // Refresh the task display after toggling completion
    refresh();
}

void update_task_recurrence(Task *task) {
    if (strcmp(task->recurrence, "none") == 0) {
        return;  // No recurrence, nothing to update
    }

    struct tm due_date = {0};
    int year, month, day;

    // Ensure the due date is valid
    if (sscanf(task->due_date, "%d-%d-%d", &year, &month, &day) != 3 || year < 1900) {
        // Invalid or uninitialized due date, do nothing
        return;
    }

    due_date.tm_year = year - 1900;  // tm_year is years since 1900
    due_date.tm_mon = month - 1;     // tm_mon is 0-based
    due_date.tm_mday = day;

    // Adjust the due date based on recurrence type
    if (strcmp(task->recurrence, "daily") == 0) {
        due_date.tm_mday += 1;
    } else if (strcmp(task->recurrence, "weekly") == 0) {
        due_date.tm_mday += 7;
    } else if (strcmp(task->recurrence, "biweekly") == 0) {
        due_date.tm_mday += 14;
    } else if (strcmp(task->recurrence, "monthly") == 0) {
        due_date.tm_mon += 1;
    } else if (strcmp(task->recurrence, "yearly") == 0) {
        due_date.tm_year += 1;
    }

    // Normalize the date
    mktime(&due_date);

    // Update the due date field
    snprintf(task->due_date, MAX_DATE_LEN, "%04d-%02d-%02d", 
             due_date.tm_year + 1900, 
             due_date.tm_mon + 1, 
             due_date.tm_mday);
}

int is_task_overdue(Task task) {
    if (strcmp(task.due_date, "N/A") == 0) {
        return 0;  // No due date, so not overdue
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm current_time = *localtime(&t);
    struct tm due_date = {0};

    // Parse the due date from the task
    if (sscanf(task.due_date, "%d-%d-%d", &due_date.tm_year, &due_date.tm_mon, &due_date.tm_mday) != 3) {
        return 0;  // Invalid due date format
    }

    // Adjust the year and month for struct tm format
    due_date.tm_year -= 1900;
    due_date.tm_mon -= 1;

    // Compare the due date with the current date
    return mktime(&current_time) > mktime(&due_date);
}

int is_task_due_soon(Task task) {
    if (strcmp(task.due_date, "N/A") == 0) return 0;

    time_t t = time(NULL);
    struct tm current_time = *localtime(&t);
    struct tm due_date = {0};

    sscanf(task.due_date, "%d-%d-%d", &due_date.tm_year, &due_date.tm_mon, &due_date.tm_mday);
    due_date.tm_year -= 1900;
    due_date.tm_mon -= 1;

    double seconds_difference = difftime(mktime(&due_date), mktime(&current_time));
    return (seconds_difference <= 86400 && seconds_difference >= 0);  // Task due in the next 24 hours
}

void add_task(Task tasks[], int *count, const char *title, const char *category, const char *due_date, const char *recurrence, int priority) {
    if (*count >= MAX_TASKS) return;

    char temp_title[MAX_TITLE_LEN];
    char temp_category[MAX_CATEGORY_LEN];
    char temp_due_date[MAX_DATE_LEN];
    int temp_priority;

    // Title input with retry if blank
    while (1) {
        get_input_and_clear(temp_title, MAX_TITLE_LEN, "Enter task title (cannot be empty): ");
        if (strlen(temp_title) > 0) break;
        mvprintw(LINES - 2, 0, "Task title cannot be empty. Please try again.");
        refresh();
        getch();
    }

    // Category input with retry if blank
    while (1) {
        get_input_and_clear(temp_category, MAX_CATEGORY_LEN, "Enter category (cannot be empty): ");
        if (strlen(temp_category) > 0) break;
        mvprintw(LINES - 2, 0, "Category cannot be empty. Please try again.");
        refresh();
        getch();
    }

    // Due date input, default to current date if blank
    get_input_and_clear(temp_due_date, MAX_DATE_LEN, "Enter due date (YYYY-MM-DD) or leave blank for today's date: ");
    if (strlen(temp_due_date) == 0) {
        // Default to current date
        time_t t = time(NULL);
        struct tm *current_time = localtime(&t);
        snprintf(temp_due_date, MAX_DATE_LEN, "%04d-%02d-%02d", 
                 current_time->tm_year + 1900, 
                 current_time->tm_mon + 1, 
                 current_time->tm_mday);
    }

    // Priority input with retry if invalid
    mvprintw(LINES - 2, 0, "Enter priority (1-5): ");
    clrtoeol();
    echo();
    scanw("%d", &temp_priority);
    noecho();
    while (temp_priority < 1 || temp_priority > 5) {
        mvprintw(LINES - 2, 0, "Invalid priority. Please enter a value between 1 and 5: ");
        clrtoeol();
        echo();
        scanw("%d", &temp_priority);
        noecho();
    }

    // Save the task using validated inputs
    tasks[*count].id = *count + 1;  // Assign a unique ID
    strncpy(tasks[*count].title, temp_title, MAX_TITLE_LEN - 1);
    tasks[*count].title[MAX_TITLE_LEN - 1] = '\0';
    strncpy(tasks[*count].category, temp_category, MAX_CATEGORY_LEN - 1);
    tasks[*count].category[MAX_CATEGORY_LEN - 1] = '\0';
    strncpy(tasks[*count].due_date, temp_due_date, MAX_DATE_LEN - 1);
    tasks[*count].due_date[MAX_DATE_LEN - 1] = '\0';
    strncpy(tasks[*count].recurrence, recurrence, MAX_RECURRENCE_LEN - 1);
    tasks[*count].recurrence[MAX_RECURRENCE_LEN - 1] = '\0';
    tasks[*count].priority = temp_priority;
    tasks[*count].completed = 0;

    (*count)++;

    mvprintw(LINES - 2, 0, "Task added successfully! Press any key...");
    clrtoeol();
    refresh();
    getch();
}

void remove_task(Task tasks[], int *count, int index) {
    if (index < 0 || index >= *count) {
        // Index out of bounds
        return;
    }

    for (int i = index; i < *count - 1; i++) {
        tasks[i] = tasks[i + 1];  // Shift tasks down
    }
    (*count)--;
}

void edit_task(Task *task) {
    char title[MAX_TITLE_LEN];
    char category[MAX_CATEGORY_LEN];
    char due_date[MAX_DATE_LEN];
    char recurrence[MAX_RECURRENCE_LEN];
    int priority;

    // Get input for task title
    get_input_and_clear(title, MAX_TITLE_LEN, "Edit task title (leave blank to keep current): ");
    if (strlen(title) > 0) {
        strncpy(task->title, title, MAX_TITLE_LEN - 1);
        task->title[MAX_TITLE_LEN - 1] = '\0';  // Ensure null termination
    }

    // Get input for category
    get_input_and_clear(category, MAX_CATEGORY_LEN, "Edit category (leave blank to keep current): ");
    if (strlen(category) > 0) {
        strncpy(task->category, category, MAX_CATEGORY_LEN - 1);
        task->category[MAX_CATEGORY_LEN - 1] = '\0';
    }

    // Get input for due date
    get_input_and_clear(due_date, MAX_DATE_LEN, "Edit due date (YYYY-MM-DD, leave blank to keep current): ");
    if (strlen(due_date) > 0) {
        strncpy(task->due_date, due_date, MAX_DATE_LEN - 1);
        task->due_date[MAX_DATE_LEN - 1] = '\0';
    }

    // Get input for recurrence
    get_input_and_clear(recurrence, MAX_RECURRENCE_LEN, "Edit recurrence (none, daily, weekly, biweekly, monthly, yearly): ");
    if (strlen(recurrence) > 0) {
        strncpy(task->recurrence, recurrence, MAX_RECURRENCE_LEN - 1);
        task->recurrence[MAX_RECURRENCE_LEN - 1] = '\0';
    }

    // Get input for priority
    mvprintw(LINES - 2, 0, "Edit priority (1-5, leave blank to keep current): ");
    clrtoeol();
    echo();
    char priority_input[3];  // Input for priority as a string
    getnstr(priority_input, sizeof(priority_input) - 1);
    noecho();

    if (strlen(priority_input) > 0) {
        priority = atoi(priority_input);
        if (priority >= 1 && priority <= 5) {
            task->priority = priority;
        }
    }

    mvprintw(LINES - 2, 0, "Task edited successfully! Press any key...");
    clrtoeol();
    refresh();
    getch();
}

void search_task(Task tasks[], int count, int *selected_task) {
    char search_query[MAX_TITLE_LEN];

    // Prompt the user to enter the search query (event name)
    get_input_and_clear(search_query, MAX_TITLE_LEN, "Enter event name to search: ");
    
    // Search through tasks
    for (int i = 0; i < count; i++) {
        if (strstr(tasks[i].title, search_query) != NULL) {
            *selected_task = i;  // Set selected task to the found event
            return;  // Exit once the event is found and selected
        }
    }

    // If no event is found
    mvprintw(LINES - 2, 0, "Event not found. Press any key to continue.");
    clrtoeol();
    refresh();
    getch();  // Wait for user to acknowledge
}

void load_tasks(Task tasks[], int *count) {
    char *file_path = get_database_path();
    FILE *file = fopen(file_path, "r");

    if (file == NULL) return;

    while (fscanf(file, "%d\t%[^\t]\t%[^\t]\t%d\t%d\t%[^\n]\n", 
                  &tasks[*count].id, tasks[*count].title,
                  tasks[*count].category, &tasks[*count].priority,
                  &tasks[*count].completed, tasks[*count].due_date) != EOF) {
        (*count)++;
    }

    fclose(file);
}

void save_tasks(Task tasks[], int count) {
    char *file_path = get_database_path();
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        // Handle file creation failure
        mvprintw(LINES - 2, 0, "Error: Could not open file for saving tasks.");
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%d\t%s\t%s\t%d\t%d\t%s\n", tasks[i].id, tasks[i].title, tasks[i].category,
                tasks[i].priority, tasks[i].completed, tasks[i].due_date);
    }

    fclose(file);
}

void display_tasks(Task tasks[], int count, int selected) {
    clear();  // Clear the screen for updating
    for (int i = 0; i < count; i++) {
        if (i == selected) {
            attron(A_REVERSE);
        }
        if (is_task_overdue(tasks[i])) {
            attron(COLOR_PAIR(1));  // Red for overdue tasks
        } else if (is_task_due_soon(tasks[i])) {
            attron(COLOR_PAIR(2));  // Yellow for due soon tasks
        }

        mvprintw(i, 0, "[%c] %s (%s) Priority: %d Due: %s Recurrence: %s", 
                 tasks[i].completed ? 'X' : ' ', tasks[i].title,
                 tasks[i].category, tasks[i].priority, tasks[i].due_date, tasks[i].recurrence);

        if (is_task_overdue(tasks[i])) {
            attroff(COLOR_PAIR(1));  // Turn off overdue color
        } else if (is_task_due_soon(tasks[i])) {
            attroff(COLOR_PAIR(2));  // Turn off due soon color
        }

        if (i == selected) {
            attroff(A_REVERSE);
        }
    }
    mvprintw(count + 1, 0, "Press 'q' to quit, 'a' to add, 'd' to delete, 'c' to complete, 'e' to edit.");
    refresh();
}
