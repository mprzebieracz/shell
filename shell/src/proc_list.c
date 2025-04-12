#include <stdlib.h>

#include "proc_list.h"

proc_node* proc_node_init(int pid) {
    proc_node* node = (proc_node*)malloc(sizeof(proc_node));
    node->pid = pid;
    node->terminated = 0;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void proc_node_destroy(proc_node* node) {
    free(node);
}

proc_list* proc_list_init() {
    proc_list* list = (proc_list*)malloc(sizeof(proc_list));
    list->head = proc_node_init(-1);
    list->tail = proc_node_init(-1);
    list->head->next = list->tail;
    list->tail->prev = list->head;
    return list;
}

void proc_list_push(proc_list* list, int pid) {
    proc_node* node = proc_node_init(pid);
    node->next = list->tail;
    node->prev = list->tail->prev;
    list->tail->prev->next = node;
    list->tail->prev = node;
}

void proc_list_destroy(proc_list* list) {
    proc_node* node = list->head->next;
    while (node != list->tail) {
        proc_node* next = node->next;
        proc_node_destroy(node);
        node = next;
    }
    proc_node_destroy(list->head);
    proc_node_destroy(list->tail);
    free(list);
}

proc_node* proc_list_find(proc_list* list, int pid) {
    proc_node* node = list->head->next;
    while (node != list->tail) {
        if (node->pid == pid) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}

proc_node* proc_list_remove(proc_list* list, int pid) {
    proc_node* node = list->head->next;
    while (node != list->tail) {
        if (node->pid == pid) {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            return node;
        }
        node = node->next;
    }
    return NULL;
}