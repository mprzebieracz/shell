#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#define BUILTIN_ERROR 2

typedef struct {
	char* name;
	int (*fun)(char**); 
} builtin_pair;

extern builtin_pair builtins_table[];

int is_builtin(char* program);

#endif /* !_BUILTINS_H_ */
