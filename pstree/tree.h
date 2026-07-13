// In this file, we will implement a tree structure .

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHILDREN 1024

// the element of process list
struct process_info {
int pid;
int ppid;
char command[256]; // some command may be longer than 256, but for simplicity we limit it here
};

struct tree
{
    int pid; // process id 
    int ppid; // parent process id 
    char command[256]; // command name
    struct tree *childrens[MAX_CHILDREN]; // array of pointers to children
    int num_children; // number of children
    struct tree *parent; // pointer to parent
};

void create_tree(struct tree *root, struct process_info *process_list, int num_processes, int numeric_sort);

void print_tree(struct tree *root, int show_pids, int col);

int compare_by_pid(const void *a, const void *b);

int compare_by_command(const void *a, const void *b);