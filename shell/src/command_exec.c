#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>

#include "utils.h"
#include "config.h"
#include "builtins.h"
#include "proc_list.h"
#include "proc_handling.h"

int exec_pipelineseq(pipelineseq* ln);
int exec_pipeline(pipeline* pl);
int exec_command(command* com);
int exec_builtin_command(command* com);
int create_pipe(int pipefds[], int i, int cmd_count);
void close_pipes(int prev_pipefd[], int curr_pipefd[]);
void update_pipes(int prev_pipefd[], int curr_pipefd[]);
pid_t fork_and_exec(command* com, int prev_pipefd[], int curr_pipefd[], int cmd_count, int i, int background);
void handle_redirseq(redirseq* redirs);
void handle_redir(redir* redir);
void print_exec_error(char* program);
void print_open_error(char* filename);

int exec_pipelineseq(pipelineseq* ln) {
    int status = 0;
    pipelineseq* ps = ln;

    status = check_pipelineseq(ln);
    if (status != OK) return status; 

    do {
        status = exec_pipeline(ps->pipeline);
        ps = ps->next;
    } while (ps != ln);

    return status;
}

int exec_pipeline(pipeline* pl) {
    commandseq* commands = pl->commands;
    int cmd_count = count_commands(commands);
    int status;
    int background = pl->flags & INBACKGROUND;
    fg_proc_cnt = 0;

    if (cmd_count == 0) {
        return OK;
    }

    if (cmd_count == 1 && commands->com == NULL) {
        return OK;
    }

    if (cmd_count == 1 && is_builtin(commands->com->args->arg)) {
        status = exec_builtin_command(commands->com);
        if (status == BUILTIN_ERROR) {
            fprintf(stderr, "Builtin %s error.\n", commands->com->args->arg);
            return FAIL;
        }
        return OK;
    }

    int prev_pipefd[2] = {-1, -1};
    sigset_t oldmask;
    int i = 0;

    block_sigchld(&oldmask);
    do {
        int curr_pipefd[2] = {-1, -1};
        if (create_pipe(curr_pipefd, i, cmd_count) == FAIL) {
            close_pipes(prev_pipefd, curr_pipefd);
            unblock_sigchld(&oldmask);
            return FAIL;
        }

        pid_t pid = fork_and_exec(commands->com, prev_pipefd, curr_pipefd, cmd_count, i++, background);
        if (pid < 0) {
            close_pipes(prev_pipefd, curr_pipefd);
            unblock_sigchld(&oldmask);
            return FAIL;
        }

        update_pipes(prev_pipefd, curr_pipefd);

        commands = commands->next;
    } while (commands != pl->commands);
    unblock_sigchld(&oldmask);

    close_pipes(prev_pipefd, NULL);

    if (!background) {
        block_sigchld(&oldmask);
        while (fg_proc_cnt) {
            sigsuspend(&oldmask);
        }
        unblock_sigchld(&oldmask);
    }
    return OK;
}

int create_pipe(int pipefds[], int i, int cmd_count) {
    if (i == cmd_count - 1) {
        pipefds[0] = -1;
        pipefds[1] = -1;
        return OK;
    }
    if (pipe(pipefds) == -1) {
        perror("pipe");
        return FAIL;
    }
    return OK;
}

void close_pipes(int prev_pipefd[], int curr_pipefd[]) {
    if (prev_pipefd != NULL) {
        if (prev_pipefd[0] != -1) close(prev_pipefd[0]);
        if (prev_pipefd[1] != -1) close(prev_pipefd[1]);
    }
    if (curr_pipefd != NULL) {
        if (curr_pipefd[0] != -1) close(curr_pipefd[0]);
        if (curr_pipefd[1] != -1) close(curr_pipefd[1]);
    }
}

void update_pipes(int prev_pipefd[], int curr_pipefd[]) {
    if (prev_pipefd[0] != -1) close(prev_pipefd[0]);
    if (prev_pipefd[1] != -1) close(prev_pipefd[1]);
    if (curr_pipefd[1] != -1) close(curr_pipefd[1]);
    
    prev_pipefd[0] = curr_pipefd[0];
    prev_pipefd[1] = -1;
    curr_pipefd[0] = -1;
}

pid_t fork_and_exec(command* com, int prev_pipefd[], int curr_pipefd[], int cmd_count, int i, int background) {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        if (background) {
            setsid();
        }
        if (i != 0) {
            if (dup2(prev_pipefd[0], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXEC_FAILURE);
            }
        }
        if (i != cmd_count - 1) {
            if (dup2(curr_pipefd[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXEC_FAILURE);
            }
        }
        close_pipes(prev_pipefd, curr_pipefd);

        handle_redirseq(com->redirs);
        exit(exec_command(com));
    }
    if (background) proc_list_push(background_procs, pid);
    else fg_proc_cnt++;

    return pid;
}

int exec_command(command* com) {
    char* program = com->args->arg;
    char* args[count_args(com) + 1];
    fill_args(com, args);

    execvp(program, args);
    print_exec_error(program);
    exit(EXEC_FAILURE);
}

int exec_builtin_command(command* com) {
    char* args[count_args(com) + 1];
    fill_args(com, args);

    for (int i = 0; builtins_table[i].name != NULL; i++) {
        if (strcmp(builtins_table[i].name, args[0]) == 0) {
            return builtins_table[i].fun(args);
        }
    }

    return BUILTIN_ERROR;
}

void handle_redirseq(redirseq* redirs) {
    if (!redirs) return;
    redirseq* rs = redirs;
    do {
        handle_redir(rs->r);
        rs = rs->next;
    } while (rs != redirs);
}

void handle_redir(redir* redir) {
    int flags = redir->flags;

    int fd;
    int fdout = STDOUT_FILENO;
    if (IS_RIN(flags)) {
        fd = open(redir->filename, O_RDONLY);
        fdout = STDIN_FILENO;
    }
    else if (IS_ROUT(flags)) fd = open(redir->filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    else if (IS_RAPPEND(flags)) fd = open(redir->filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    else return;

    if (fd == -1) {
        print_open_error(redir->filename);
        exit(EXEC_FAILURE);
    }

    if (dup2(fd, fdout) == -1) {
        perror("dup2");
        exit(EXEC_FAILURE);
    }

    close(fd);
}

void print_exec_error(char* program) {
    if (errno == ENOENT) {
        fprintf(stderr, "%s%s", program, DOESNT_EXIST_STR);
    }
    else if (errno == EACCES) {
        fprintf(stderr, "%s%s", program, PERMISSION_DENIED_STR);
    }
    else {
        fprintf(stderr, "%s%s", program, EXEC_ERROR_STR);
    }
}

void print_open_error(char* filename) {
    if (errno == ENOENT) {
        fprintf(stderr, "%s%s", filename, DOESNT_EXIST_STR);
    }
    else if (errno == EACCES) {
        fprintf(stderr, "%s%s", filename, PERMISSION_DENIED_STR);
    }
    else {
        perror("open");
    }
}