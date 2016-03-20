#ifndef _TARGET_CODE_H_
#define _TARGET_CODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "intermediateCode.h"
#include "stack.h"
#include "list.h"

#define EXPAND_SIZE_T 		1024
#define CURR_SIZE_T 		(total_t*sizeof(instruction))
#define NEW_SIZE_T 			(EXPAND_SIZE_T*sizeof(instruction)+CURR_SIZE_T)

#define RESET_OPERAND(m)		memset(m, 0, sizeof(vmarg))
#define NEXTINSTRUCTIONLABEL 	currInstruction
#define CURRPROCESSEDQUAD 		currProcessed

typedef struct userfunc {
	unsigned		address;
	unsigned		localSize;
	char*			id;
	struct userfunc *next;
} userfunc;

typedef struct stringElem {
	char *s;
	struct stringElem *next;
} stringElem;

typedef struct numElem {
	double num;
	struct numElem *next;
} numElem;

typedef enum vmopcode {
	assign_v, 		add_v,			sub_v,
	mul_v,			div_v,			mod_v,
	jeq_v,			jne_v,			jle_v,
	jge_v,			jlt_v,			jgt_v,
	call_v,			pusharg_v,		getretval_v,
	return_v,		funcenter_v,	funcexit_v,		
	newtable_v,		tablegetelem_v,	tablesetelem_v,	
	jump_v, 		nop_v
} vmopcode;

typedef enum vmarg_t {
	label_a 		=0,
	global_a		=1,
	formal_a		=2,
	local_a			=3,
	number_a		=4,
	string_a		=5,
	bool_a			=6,
	nil_a			=7,
	userfunc_a		=8,
	libfunc_a		=9,
	retval_a		=10
} vmarg_t;

typedef struct vmarg {
	vmarg_t		type;
	unsigned	val;
} vmarg;

typedef struct instruction {
		vmopcode	opcode;
		vmarg 		result;
		vmarg 		arg1;
		vmarg 		arg2;
		unsigned 	srcLine;
} instruction;

typedef struct incomplete_jump{
	unsigned instrNo;
	unsigned iaddress;
	struct incomplete_jump * next;
} incomplete_jump;

void 			generator (void);
void 			WriteTargetCode(void);
void 			make_operand (expr* e, vmarg* arg);
void 			make_numberoperand (vmarg* arg, double val);
void 			make_booloperand (vmarg* arg, unsigned val);
void 			make_retvaloperand(vmarg* arg);
unsigned 		consts_newstring(char *s);
unsigned 		consts_newnumber(double n);
unsigned 		consts_newlibfunc(char *s);
void 			generate (vmopcode op, quad *q);
void 			generate_relational (vmopcode op, quad *q);
void 			patch_incomplete_jumps();
void 			expand_t();
void 			emit_t(instruction * t);
returnElem * 	backpatchReturn(returnElem *head, unsigned label);
unsigned 		funcIndex(char *s);

extern void generate_ADD(quad *q);
extern void generate_SUB(quad *q);
extern void generate_MUL(quad *q);
extern void generate_DIV(quad *q);
extern void generate_MOD(quad *q);
extern void generate_NEWTABLE(quad *q);
extern void generate_TABLEGETELEM(quad *q);
extern void generate_TABLESETELEM(quad *q);
extern void generate_ASSIGN(quad *q);
extern void generate_JUMP(quad *q);
extern void generate_IF_EQ(quad *q);
extern void generate_IF_NOTEQ(quad *q);
extern void generate_IF_GREATER(quad *q);
extern void generate_IF_GREATEREQ(quad *q);
extern void generate_IF_LESS(quad *q);
extern void generate_IF_LESSEQ(quad *q);
extern void generate_PARAM(quad *q);
extern void generate_CALL(quad *q);
extern void generate_GETRETVAL(quad *q);
extern void generate_FUNCSTART(quad *q);
extern void generate_RETURN(quad *q);
extern void generate_FUNCEND(quad *q);
extern void generate_NOP(quad *q);

extern unsigned 	currInstruction;
extern unsigned		totalNumConsts;
extern unsigned 	totalStringConsts;
extern unsigned 	totalNamedLibfuncs;
extern unsigned		totalUserFuncs;
extern unsigned		MagicNumber;

extern numElem * 		numConsts;
extern stringElem *		stringConsts;
extern stringElem *		namedLibfuncs;
extern userfunc *		userFuncs;
extern instruction * 	Instructions;

void printInstructions();
void print_instruction();

#endif
