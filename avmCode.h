#ifndef _AVM_CODE_H_
#define _AVM_CODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "targetCode.h"

#define AVM_STACKSIZE 		4096
#define AVM_TABLE_HASHSIZE 	211
#define AVM_WIPEOUT(m) 	memset(&(m), 0, sizeof(m))

#define AVM_MAX_INSTRUCTIONS 	(unsigned) nop_v
#define AVM_ENDING_PC 			currInstruction
#define AVM_NUMACTUALS_OFFSET	4
#define AVM_SAVEDPC_OFFSET		3
#define AVM_SAVEDTOP_OFFSET		2
#define AVM_SAVEDTOPSP_OFFSET	1
#define AVM_STACKENV_SIZE	4

#define avm_error(...) 	({ \
						printf("avm error: "); \
						printf(__VA_ARGS__); \
						assert(0); \
						})

#define avm_warning		printf("avm warning: ");printf

#define execute_add execute_arithmetic
#define execute_sub execute_arithmetic
#define execute_mul execute_arithmetic
#define execute_div execute_arithmetic
#define execute_mod execute_arithmetic

#define execute_jle execute_comparison
#define execute_jge execute_comparison
#define execute_jlt execute_comparison
#define execute_jgt execute_comparison

typedef enum avm_memcell_t {
	number_m		=0,
	string_m		=1,
	bool_m			=2,
	table_m			=3,
	userfunc_m		=4,
	libfunc_m		=5,
	nil_m			=6,
	undef_m			=7
} avm_memcell_t;

typedef struct avm_memcell {
	avm_memcell_t type;
	union {
		double				numVal;
		char*				strVal;
		unsigned char	 	boolVal;
		struct avm_table*	tableVal;
		unsigned			funcVal;
		char*				libfuncVal;
	} data;
} avm_memcell;

typedef struct avm_table_bucket {
	avm_memcell 					key;
	avm_memcell 					value;
	struct avm_table_bucket *		next;
} avm_table_bucket;

typedef struct avm_table {
	unsigned 					refCounter;
	struct avm_table_bucket *	strIndexed[AVM_TABLE_HASHSIZE];
	struct avm_table_bucket *	numIndexed[AVM_TABLE_HASHSIZE];
	unsigned 					total;
} avm_table;

typedef void 	(*execute_func_t)		(instruction *m);
typedef void 	(*memclear_func_t)		(avm_memcell *m);
typedef void 	(*library_func_t)		(void);
typedef char * 	(*tostring_func_t)		(avm_memcell *m);
typedef double 	(*arithmetic_func_t) 	(double x,double y);
typedef unsigned char (*tobool_func_t)	(avm_memcell *m);
typedef unsigned char (*cmp_func_t)		(double x, double y);
typedef unsigned (*tonum_func_t)		(avm_memcell *m);

library_func_t 	avm_getlibraryfunc(char *id);

extern void 		avm_assign(avm_memcell *lv, avm_memcell *rv);
extern char* 		avm_tostring(avm_memcell *);
extern unsigned char avm_tobool (avm_memcell *m);
extern unsigned 	avm_tonum(avm_memcell *);
extern void			avm_calllibfunc(char *funcName);
extern void			avm_callsaveenvironment(void);
extern userfunc* 	avm_getfuncinfo(unsigned);

unsigned 		avm_hash(avm_memcell *i);
unsigned 		avm_get_envvalue(unsigned i);
void 			avm_dec_top(void);
void 			avm_push_envvalue(unsigned val);
void 			avm_callsaveenvironment(void);
unsigned		avm_totalactuals(void);
avm_memcell *	avm_getactual(unsigned);
void			avm_registerlibfunc(char *id, library_func_t addr);
void 			libfunc_print(void);
void 			read_from_binary(void);
static void 	avm_initstack(void);
avm_table* 		avm_tablenew(void);
void			avm_tabledestroy(avm_table *t);
avm_memcell* 	avm_tablegetelem(avm_table* table, avm_memcell* index);
void			avm_tablesetelem(avm_table *table, avm_memcell *index, avm_memcell *content);
avm_memcell*	avm_translate_operand(vmarg *arg, avm_memcell* reg);
void			avm_memcellclear(avm_memcell *m);
void 			avm_tablebucketsinit(avm_table_bucket **p);
void			avm_tableincrefcounter(avm_table *t);
void			avm_tabledecrefcounter(avm_table *t);
double			consts_getnumber (unsigned index);
char*			consts_getstring (unsigned index);
unsigned 		userfunc_getaddress(unsigned index);
char*			libfuncs_getused (unsigned index);
unsigned 		string_to_number(char * str);

unsigned 		execute_cycle(void);
void 			execute_comparison(instruction* inst);

extern void execute_assign(instruction *instr);
extern void execute_add(instruction *instr);
extern void execute_sub(instruction *instr);
extern void execute_mul(instruction *instr);
extern void execute_div(instruction *instr);
extern void execute_mod(instruction *instr);
extern void execute_jeq(instruction *instr);
extern void execute_jne(instruction *instr);
extern void execute_jle(instruction *instr);
extern void execute_jge(instruction *instr);
extern void execute_jlt(instruction *instr);
extern void execute_jgt(instruction *instr);
extern void execute_call(instruction *instr);
extern void execute_pusharg(instruction *instr);
extern void execute_funcenter(instruction *instr);
extern void execute_funcexit(instruction *instr);
extern void execute_newtable(instruction *instr);
extern void execute_tablegetelem(instruction *instr);
extern void execute_tablesetelem(instruction *instr);
extern void execute_nop(instruction *instr);
extern void execute_jump(instruction *instr);

extern char * number_tostring   (avm_memcell *m);
extern char * string_tostring   (avm_memcell *m);
extern char * bool_tostring     (avm_memcell *m);
extern char * table_tostring    (avm_memcell *m);
extern char * userfunc_tostring (avm_memcell *m);
extern char * libfunc_tostring  (avm_memcell *m);
extern char * nil_tostring      (avm_memcell *m);
extern char * undef_tostring    (avm_memcell *m);

extern unsigned char number_tobool	(avm_memcell *m);
extern unsigned char string_tobool	(avm_memcell *m);
extern unsigned char bool_tobool	(avm_memcell *m);
extern unsigned char table_tobool	(avm_memcell *m);
extern unsigned char userfunc_tobool(avm_memcell *m);
extern unsigned char libfunc_tobool	(avm_memcell *m);
extern unsigned char nil_tobool		(avm_memcell *m);
extern unsigned char undef_tobool	(avm_memcell *m);

extern unsigned number_tonum	(avm_memcell *m);
extern unsigned string_tonum	(avm_memcell *m);
extern unsigned bool_tonum		(avm_memcell *m);
extern unsigned table_tonum		(avm_memcell *m);
extern unsigned userfunc_tonum	(avm_memcell *m);
extern unsigned libfunc_tonum	(avm_memcell *m);
extern unsigned nil_tonum		(avm_memcell *m);
extern unsigned undef_tonum		(avm_memcell *m);

extern void libfunc_print(void);
extern void libfunc_input(void);
extern void libfunc_objectmemberkeys(void);
extern void libfunc_objecttotalmembers(void);
extern void libfunc_objectcopy(void);
extern void libfunc_totalarguments(void);
extern void libfunc_argument(void);
extern void libfunc_typeof(void);
extern void libfunc_strtonum(void);
extern void libfunc_sqrt(void);
extern void libfunc_cos(void);
extern void libfunc_sin(void);

extern double add_impl(double x, double y);
extern double sub_impl(double x, double y);
extern double mul_impl(double x, double y);
extern double div_impl(double x, double y);
extern double mod_impl(double x, double y);

extern unsigned char jge_impl(double x, double y);
extern unsigned char jgt_impl(double x, double y);
extern unsigned char jle_impl(double x, double y);
extern unsigned char jlt_impl(double x, double y);

extern void memclear_string (avm_memcell *m){
	assert(m->data.strVal);
	free(m->data.strVal);
}
extern void memclear_table (avm_memcell *m){
	assert(m->data.tableVal);
	avm_tabledecrefcounter(m->data.tableVal);
}

#endif
