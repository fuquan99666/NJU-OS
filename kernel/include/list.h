// List data structure for managing memory blocks
#include <common.h>

typedef struct node {
    uintptr_t low; // the low address of the memory block
    uintptr_t high; // the high address of the memory block
    size_t size; // the size of the memory block
    struct node *next;
    struct node *prev;
} node;

typedef struct List {
    struct node *head;
    int count; // the number of free memory blocks
} List;


// basic operations for the doubly linked list
void delete_node(node *n, List *list);

void insert_node(node *n, List *list);