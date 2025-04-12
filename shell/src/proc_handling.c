#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include "proc_list.h"
#include "proc_handling.h"

proc_list* background_procs = NULL;
volatile int fg_proc_cnt = 0;

void print_status(int pid, int status) {
    if (WIFEXITED(status)) {
        printf("Background process %d terminated. (exited with status %d)\n", pid, WEXITSTATUS(status));
    } 
    else if (WIFSIGNALED(status)) {
        printf("Background process %d terminated. (killed by signal %d)\n", pid, WTERMSIG(status));
    }
}

void print_finished_procs() {
    sigset_t oldmask;
    struct stat statbuf;
    if (fstat(STDIN_FILENO, &statbuf) == -1) {
        exit(EXIT_FAILURE);
    }
    block_sigchld(&oldmask);
    proc_node* node = background_procs->head->next;
    while (node != background_procs->tail) {
        if (node->terminated) {
            if (S_ISCHR(statbuf.st_mode)) {
                print_status(node->pid, node->status);
            }
            proc_node* next = node->next;
            proc_node* prev = node->prev;
            prev->next = next;
            next->prev = prev;
            proc_node_destroy(node);
        }
        node = node->next;
    }
    unblock_sigchld(&oldmask);
}

void sigchld_handler(int signo) {
    int status;
    int pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        proc_node* node = proc_list_find(background_procs, pid);
        if (node) {
            node->terminated = 1;
            node->status = status;
        }
        else fg_proc_cnt--;
    }
}

void block_sigchld(sigset_t* oldmask) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, oldmask);
}

void unblock_sigchld(sigset_t* oldmask) {
    sigprocmask(SIG_SETMASK, oldmask, NULL);
}

void set_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);
}

