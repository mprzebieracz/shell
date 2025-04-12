#ifndef PROC_LIST_H
#define PROC_LIST_H

#include "siparse.h"

typedef struct proc_node {
    int pid;
    int status;
    int terminated;
    struct proc_node* next;
    struct proc_node* prev;
} proc_node;

typedef struct proc_list {
    proc_node* head;
    proc_node* tail;
} proc_list;

extern proc_list* background_procs;

proc_node* proc_node_init(int pid);
void proc_node_destroy(proc_node* node);

proc_list* proc_list_init();
void proc_list_push(proc_list* list, int pid);
proc_node* proc_list_find(proc_list* list, int pid);
void proc_list_destroy(proc_list* list);
proc_node* proc_list_remove(proc_list* list, int pid);


#endif // PROC_LIST_H