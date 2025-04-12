#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"
#include "config.h"
#include "line_parse.h"
#include "command_exec.h"
#include "proc_list.h"
#include "proc_handling.h"

int main(int argc, char* argv[]) {
    pipelineseq* ln;
    ssize_t bytes_read;
    char line[MAX_LINE_LENGTH + 1];
    int status;
    background_procs = proc_list_init(); 

    set_sigchld_handler();
    signal(SIGINT, SIG_IGN);

    while (1) {
        print_finished_procs();
        print_prompt();

        bytes_read = get_line(line);
        if (bytes_read == -1) continue;
        if (bytes_read == 0) break;

        ln = parseline(line);

        status = exec_pipelineseq(ln); // TODO: error handling
    }

    proc_list_destroy(background_procs);
    return EXIT_SUCCESS;
}