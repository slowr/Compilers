#ifndef _STACK_H_
#define _STACK_H_

#include <stdlib.h>
#include "symtable.h"

typedef struct stack {
	int scope;
	struct stack *next;
} Stack;

int 	isEmpty(Stack *head);
void 	push (Stack ** head, int scope);
int 	pop (Stack ** head);

typedef struct stack_func {
	SymbolTableEntry *data;
	struct stack_func *next;
} stack_data;

typedef struct funcstack{
	size_t size;
	stack_data *stack;
} funcstack;

funcstack * 		newStack_func();
void 				push_func(funcstack *st, SymbolTableEntry *entry);
SymbolTableEntry * 	pop_func(funcstack *st);
SymbolTableEntry *	top_func(funcstack *st);
int 				is_empty_func(funcstack *st);
int 				is_full_func(funcstack *st);
int 				size_func(funcstack *st);

#endif
