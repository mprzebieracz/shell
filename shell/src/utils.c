#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "siparse.h"
#include "builtins.h"
#include <string.h>

void printcommand(command* pcmd, int k) {
	int flags;

	printf("\tCOMMAND %d\n", k);
	if (pcmd == NULL) {
		printf("\t\t(NULL)\n");
		return;
	}

	printf("\t\targv=:");
	argseq* argseq = pcmd->args;
	do {
		printf("%s:", argseq->arg);
		argseq = argseq->next;
	} while (argseq != pcmd->args);

	printf("\n\t\tredirections=:");
	redirseq* redirs = pcmd->redirs;
	if (redirs) {
		do {
			flags = redirs->r->flags;
			printf("(%s,%s):", redirs->r->filename, IS_RIN(flags) ? "<" : IS_ROUT(flags) ? ">" : IS_RAPPEND(flags) ? ">>" : "??");
			redirs = redirs->next;
		} while (redirs != pcmd->redirs);
	}

	printf("\n");
}

void printpipeline(pipeline* p, int k) {
	int c;
	command** pcmd;

	commandseq* commands = p->commands;

	printf("PIPELINE %d\n", k);

	if (commands == NULL) {
		printf("\t(NULL)\n");
		return;
	}
	c = 0;
	do {
		printcommand(commands->com, ++c);
		commands = commands->next;
	} while (commands != p->commands);

	printf("Totally %d commands in pipeline %d.\n", c, k);
	printf("Pipeline %sin background.\n", (p->flags & INBACKGROUND) ? "" : "NOT ");
}

void printparsedline(pipelineseq* ln) {
	int c;
	pipelineseq* ps = ln;

	if (!ln) {
		printf("%s\n", SYNTAX_ERROR_STR);
		return;
	}
	c = 0;

	do {
		printpipeline(ps->pipeline, ++c);
		ps = ps->next;
	} while (ps != ln);

	printf("Totally %d pipelines.", c);
	printf("\n");
}

command* pickfirstcommand(pipelineseq* ppls) {
	if ((ppls == NULL)
		|| (ppls->pipeline == NULL)
		|| (ppls->pipeline->commands == NULL)
		|| (ppls->pipeline->commands->com == NULL))	return NULL;

	return ppls->pipeline->commands->com;
}

int check_pipeline(pipeline* pl) {
    commandseq* commands = pl->commands;
    int len = 0;
    int null_cmd = 0;

    do {
        len++;
        if (commands->com == NULL) null_cmd++;
    } while (commands != pl->commands);

    if (len > 1 && null_cmd > 0) {
        printf("%s\n", SYNTAX_ERROR_STR);
        return FAIL;
    }
    return OK;
}

int check_pipelineseq(pipelineseq* ln) {
    pipelineseq* ps = ln;
    int status;

    if (!ln) {
        printf("%s\n", SYNTAX_ERROR_STR);
        return FAIL;
    }

    do {
        status = check_pipeline(ps->pipeline); // TODO: error handling
        ps = ps->next;
    } while (ps != ln && status == OK);

    return status;
}

int count_args(command* com) {
    int count = 0;
    argseq* argseq = com->args;
    do {
        count++;
        argseq = argseq->next;
    } while (argseq != com->args);

    return count;
}

void fill_args(command* com, char* args[]) {
    int i = 0;
    argseq* argseq = com->args;

    do {
        args[i++] = argseq->arg;
        argseq = argseq->next;
    } while (argseq != com->args);

    args[i] = NULL;
}

int count_commands(commandseq* commands) {
    int count = 0;
    commandseq* comseq = commands;
    do {
        count++;
        comseq = comseq->next;
    } while (comseq != commands);

    return count;
}

void print_prompt() {
    struct stat statbuf;
    if (fstat(STDIN_FILENO, &statbuf) == -1) {
        exit(EXIT_FAILURE);
    }

    if (S_ISCHR(statbuf.st_mode)) {
        printf(PROMPT_STR);
        fflush(stdout);
    }
}
