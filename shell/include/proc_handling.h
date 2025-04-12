#ifndef PROC_HANDLING_H
#define PROC_HANDLING_H

#include <signal.h>

#include "proc_list.h"

extern volatile int fg_proc_cnt;

void print_finished_procs();

void sigchld_handler(int sig);

void block_sigchld(sigset_t* oldmask);
void unblock_sigchld(sigset_t* oldmask);

void set_sigchld_handler();


#endif // PROC_HANDLING_H