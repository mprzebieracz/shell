#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

#include "builtins.h"
#include "config.h"

int echo(char* []);
int lexit(char* []);
int lcd(char* []);
int lkill(char* []);
int lls(char* []);
int undefined(char* []);
int is_builtin(char* program);

builtin_pair builtins_table[] = {
	{"exit",	&lexit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

int echo(char* argv[]) {
	int i = 1;
	if (argv[i]) printf("%s", argv[i++]);
	while (argv[i])
		printf(" %s", argv[i++]);

	printf("\n");
	fflush(stdout);
	return 0;
}

int lexit(char* argv[]) {
	exit(0);
	return BUILTIN_ERROR;
}

int lcd(char* argv[]) {
	if (argv[1] == NULL) {
		// go to home directory
		if (chdir(getenv("HOME")) == -1) {
			return BUILTIN_ERROR;
		}
	}
	else if (argv[2] != NULL) {
		return BUILTIN_ERROR;
	}
	else if (chdir(argv[1]) == -1) {
		return BUILTIN_ERROR;
	}
	return 0;
}

int lls(char* argv[]) {
	char* dir_name = ".";
	if (argv[1] != NULL) {
		dir_name = argv[1];
	}

	DIR* dir = opendir(dir_name);
	if (dir == NULL) {
		return BUILTIN_ERROR;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		if (entry->d_name[0] != '.') {
			printf("%s\n", entry->d_name);
			fflush(stdout);
		}
	}

	closedir(dir);
	return 0;
}

int lkill(char* argv[]) {
	if (argv[1] == NULL) { // ok
		return BUILTIN_ERROR;
	}
	pid_t pid;
	int sig;

	char* endptr;
	errno = 0;
	long val = strtol(argv[1], &endptr, 10);
	if (val < INT_MIN || val > INT_MAX) {
		return BUILTIN_ERROR;
	}
	if (errno != 0 || *endptr != '\0') {
		return BUILTIN_ERROR;
	}
	
	if (argv[2] == NULL) {
		pid = val;
		sig = SIGTERM;
	}
	else {
		errno = 0;
		long val2 = strtol(argv[2], &endptr, 10);
		if (val2 < INT_MIN || val2 > INT_MAX) {
			return BUILTIN_ERROR;
		}
		if (errno != 0 || *endptr != '\0') {
			return BUILTIN_ERROR;
		}
		pid = val2;
		sig = -val;
	}

	kill(pid, sig);

	return 0;
}

int undefined(char* argv[]) {
	fprintf(stderr, "Command %s undefined.\n", argv[0]);
	return BUILTIN_ERROR;
}

int is_builtin(char* program) {
	for (int i = 0; builtins_table[i].name != NULL; i++) {
		if (strcmp(builtins_table[i].name, program) == 0) {
			return 1;
		}
	}
	return 0;
}
