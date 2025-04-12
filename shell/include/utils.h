#ifndef _UTILS_H_
#define _UTILS_H_

#include "siparse.h"

void printcommand(command*, int);
void printpipeline(pipeline*, int);
void printparsedline(pipelineseq*);

command* pickfirstcommand(pipelineseq*);

int check_pipelineseq(pipelineseq* ln);

void print_prompt();
int count_args(command*);
void fill_args(command*, char*[]);
int count_commands(commandseq*);

#endif /* !_UTILS_H_ */
