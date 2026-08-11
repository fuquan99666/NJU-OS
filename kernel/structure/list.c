// list.c is a simple implementation of a doubly linked list 

#include "list.h"

// basic operations for the doubly linked list 
void delete_node(node *n, List *list) {
    if (n->prev) {
        n->prev->next = n->next;
        if (n->next) {
            n->next->prev = n->prev;
        }
    } else {
        // n is the head of the list 
        list->head = n->next;
        if (n->next) {
            n->next->prev = NULL;
        }
    }
}

void insert_node(node *n, List *list) {
    n->next = list->head;
    n->prev = NULL;
    if (list->head) {
        list->head->prev = n;
    }
    list->head = n;
}