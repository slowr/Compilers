/****************************************************************/
/* Filename: 	symtable.h - Assignment 3				 		*/
/* Author: 	Dimitris Mavrommatis (mavromat@csd.uoc.gr - ID 2961)*/
/* Description: Header file for symtables						*/
/****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef   _SYMTAB_H_
#define   _SYMTAB_H_

#define HASH_MULTIPLIER 	65599
#define EXTEND

#define DEFAULT_SIZE		509
#define	MAX_BUCKETS_SIZE	65521

#define boolean int

#ifdef EXTEND
static int buckets_sz[] = {
	509, 1021, 2053,
	4093, 8191, 16381,
	32771, 65521
};
#endif

typedef enum scopespace_t {
	programvar,
	functionlocal,
	formalarg
} scopespace_t;

typedef enum symbol_t {
	var_s,
	programfunc_s,
	libraryfunc_s
} symbol_t;

typedef enum SymbolTableType {
	GLOBAL_T = 0, 
	LOCAL_T = 1, 
	FORMAL_T = 2, 
	USERFUNC_T = 3, 
	LIBFUNC_T = 4
} SymbolTableType;

typedef struct ScopeList {
	unsigned int scope;
	struct ScopeNode *head;
	struct ScopeList *next;
} ScopeList;

typedef struct ScopeNode {
	struct SymbolTableEntry *symbol;
	struct ScopeNode *next;
} ScopeNode;

typedef struct Variable {
	char *name;
	unsigned int scope;
	unsigned int line;
} Variable;

typedef struct ListArgs {
	Variable *var;
	struct ListArgs *next;
} ListArgs;

typedef struct Function {
	char *name;
	ListArgs *args;
	unsigned int scope;
	unsigned int line;
	int iaddress;
} Function;

typedef struct returnElem {
	unsigned label;
	struct returnElem *next;
} returnElem;

typedef struct SymbolTableEntry {
  	int isActive;
	union {
		Variable *varVal;
		Function *funcVal;
	} value;
	SymbolTableType type;

	symbol_t symbolType;
	scopespace_t scopeSpace;
	unsigned offset;
	unsigned totallocals;
	unsigned taddress;
	returnElem * returnList;
  	struct SymbolTableEntry *next;
} SymbolTableEntry;

typedef struct symTable {
  	unsigned int size;
  	unsigned int length;
  	SymbolTableEntry **hashtable;
} SymTable_T;

ScopeList * insertScopeList(ScopeList *head, int scope);
void 		hide(ScopeList *head, int scope);
ScopeList * lookupScopeList(ScopeList *head, int scope);
ScopeNode * lookup(ScopeList *head, int scope, char *name);
ScopeList * insertScopeNode(ScopeList *head, SymbolTableEntry *sym);

SymTable_T * 		SymTable_new(void);
static unsigned int hash(const char *pcKey, const int buckets);
SymbolTableEntry * 	put(SymTable_T *oSymTable, ScopeList **oScopeList, char *name, unsigned int scope, unsigned int line, SymbolTableType type, symbol_t symbolType, scopespace_t scopeSpace);
SymbolTableEntry * 	SymTable_lookup(SymTable_T *oSymTable, char *name, unsigned int scope);

SymbolTableEntry * 	insertArgsFunction(SymTable_T *oSymTable, ScopeList **oScopeList, SymbolTableEntry *func, char *name, unsigned int scope, unsigned int line);

void printHashTable(SymTable_T *head);
void printScopeList(ScopeList *head);
char * printType(SymbolTableType type);

int 	getSymLine(SymbolTableEntry * sym);
int 	getSymScope(SymbolTableEntry * sym);
char *	getSymName(SymbolTableEntry * sym);

extern ScopeList * scopelist;
extern SymTable_T * symTable;
extern int curr_scope;
extern SymbolTableEntry * last_node;
extern SymbolTableType last_type;
extern char * tmp;
extern ScopeNode * scope_node;
extern int func_line;

#endif