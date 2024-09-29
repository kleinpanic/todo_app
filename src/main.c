#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "todo.h"

#define MAX_TASKS 100

Task tasks[MAX_TASKS]; 
int task_count = 0;
int selected_task = 0;

// Booleans to track sort order
bool priority_ascending = true;  // Start with highest to lowest priority
bool date_ascending = true;      // Start with closest to latest due date 

// Function to clear input prompt after getting the input
void get_input_and_clear(char *buffer, int size, const char *prompt) {
    mvprintw(LINES - 2, 0, "%s", prompt);
    clrtoeol();  // Clears the prompt area before input to avoid overlay
    echo();
    getnstr(buffer, size - 1);  // Get user input
    noecho();
    buffer[size - 1] = '\0';  // Null-terminate the string
    clear();  // Clear the entire screen after each input
    refresh();
}

void delete_task_interactive() {
    remove_task(tasks, &task_count, selected_task);  // Pass index instead of id
    if (selected_task >= task_count && task_count > 0) selected_task = task_count - 1;
}

int main() {
    init_ncurses();
    load_tasks(tasks, &task_count);

    int ch;
    display_tasks(tasks, task_count, selected_task);  // Initial display
    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'j':
                if (selected_task < task_count - 1) selected_task++;
                break;
            case 'k':
                if (selected_task > 0) selected_task--;
                break;
            case 'a':
                add_task(tasks, &task_count, "", "", "", "none", 0);
                break;
            case 'd':
                delete_task_interactive();
                break;
            case 'c':
                toggle_task_completion(&tasks[selected_task]);
                break;
            case 'e':
                edit_task(&tasks[selected_task]);
                break;
            case 's':  // Search functionality
                search_task(tasks, task_count, &selected_task);
                break;
            case 'P':  // Toggle priority sorting
                sort_tasks(tasks, task_count, 'p', priority_ascending);
                priority_ascending = !priority_ascending;  // Toggle the boolean
                selected_task = 0;  // Reset selection to the first task after sorting
                break;
            case 'S':  // Toggle due date sorting
                sort_tasks(tasks, task_count, 'd', date_ascending);
                date_ascending = !date_ascending;  // Toggle the boolean
                selected_task = 0;  // Reset selection to the first task after sorting
                break;
        }

        display_tasks(tasks, task_count, selected_task);
    }

    save_tasks(tasks, task_count);
    cleanup_ncurses();
    return 0;
}

