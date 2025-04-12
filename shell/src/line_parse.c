#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include "config.h"
#include "utils.h"

char get_next_char() {
    static char buf[MAX_LINE_LENGTH + 5];
    static ssize_t buf_size = 0;
    static ssize_t buf_pos = 0;

    if (buf_pos == buf_size) {
        buf_size = read(STDIN_FILENO, buf, MAX_LINE_LENGTH + 2);
        if (buf_size <= 0) {
            return buf_size;
        }
        buf_pos = 0;
    }

    return buf[buf_pos++];
}

int get_line(char* line) {
    ssize_t line_pos = 0;

    while (1) {
        char c = get_next_char();

        if (c == -1) {
            return -1;
        }

        if (c == 0) {
            if (line_pos > 0) {
                line[line_pos] = '\0';
                return 1;
            }
            return 0;
        }

        if (c == '\n') {
            line[line_pos] = '\0';
            return 1;
        }

        if (line_pos >= MAX_LINE_LENGTH) {
            fprintf(stderr, "%s\n", SYNTAX_ERROR_STR);
            while (1) {
                c = get_next_char();
                if (c == -1 || c == '\n') {
                    return -1;
                }
                if (c == 0) {
                    return 0;
                }
            }
        }

        line[line_pos++] = c;
    }
}