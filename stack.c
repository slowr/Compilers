#include "stack.h"

#define MAX_SIZE_STACK 1024

int isEmpty(Stack *head){
	return (head == NULL);
}

void push (Stack ** head, int scope)
{
	Stack *tmp;
	tmp = (Stack * )malloc(sizeof(Stack));
	tmp -> scope = scope;
	
	if (*head == NULL)
	{
		tmp ->next  = NULL;
		*head = tmp;
	}
	else 
	{
		tmp -> next = *head;
		*head = tmp;
	}
}

int pop (Stack **head)
{
	Stack *tmp;
	int scope;
	
	if (*head == NULL)
		return -1;
	
	tmp = *head;
	*head = (*head) -> next;
	scope = tmp -> scope;
	free(tmp);
	
	return scope;
}

funcstack * newStack_func() {
	funcstack *tmp = (funcstack *) malloc (sizeof(funcstack));

	tmp -> size = 0;
	tmp -> stack = NULL;

	return tmp;
}
void push_func (funcstack *st, SymbolTableEntry *entry) {
	stack_data *tmp = (stack_data *) malloc (sizeof (stack_data));
	tmp -> data = entry;
	if (st -> stack == NULL){
		tmp -> next = NULL;
	} else {
		tmp -> next = st -> stack;
	}
	st -> stack = tmp;
	st -> size ++;
}

SymbolTableEntry * pop_func (funcstack *st){
	stack_data *tmp;
	SymbolTableEntry *sym;
	
	if (is_empty_func(st)){
		return NULL;
	}

	tmp = st -> stack;
	st -> stack = st -> stack -> next;
	sym = tmp -> data;
	free (tmp);
	st -> size --;

	return sym;
}

SymbolTableEntry *top_func (funcstack *st) {
	if (is_empty_func(st)){
		return NULL;
	} else {
		return st -> stack -> data;
	}
}

boolean is_empty_func (funcstack *st){
	return ( st -> size == 0);
 
}

boolean is_full_func (funcstack *st){
	return (st -> size == MAX_SIZE_STACK);
}

int size_func (funcstack *st){
	return st -> size;
}
