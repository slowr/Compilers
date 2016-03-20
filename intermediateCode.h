#ifndef _INTERMEDIATE_CODE_H_
#define _INTERMEDIATE_CODE_H_

#include <assert.h>
#include "symtable.h"

#define EXPAND_SIZE_I 1024
#define CURR_SIZE_I (total_i*sizeof(quad))
#define NEW_SIZE_I (EXPAND_SIZE_I*sizeof(quad)+CURR_SIZE_I)

#define ENTERSCOPESPACE 			++scopeSpaceCounter
#define EXITSCOPESPACE 				--scopeSpaceCounter
#define RESETFORMALARGSOFFSET 		formalArgOffset = 0
#define RESETFUNCTIONLOCALOFFSET 	functionLocalOffset = 0
#define NEXTQUAD 					currQuad

typedef enum iopcode {
	assign,
	add,
	sub,
	mul,
	divi,
	mod,
	if_eq,
	if_noteq,
	if_lesseq,
	if_greatereq,
	if_less,
	if_greater,
	call,
	param,
	ret,
	getretval,
	funcstart,
	funcend,
	tablecreate,
	tablegetelem,
	tablesetelem,
	jump
} iopcode;

typedef enum expr_t {
	var_e,
	tableitem_e,
	programfunc_e,
	libraryfunc_e,
	arithexpr_e,
	boolexpr_e,
	assignexpr_e,
	newtable_e,
	constnum_e,
	constbool_e,
	conststring_e,
	nil_e
} expr_t;

typedef struct booleanlist {
	unsigned value;
	struct booleanlist *next;
} booleanlist;

typedef struct expr{
	expr_t type;
	SymbolTableEntry *sym;
	struct expr *index;
	double numConst;
	char *strConst;
	unsigned char boolConst;
	struct expr *next;
	booleanlist *truelist;
	booleanlist *falselist;
} expr;

typedef struct func {
	expr *elist;
	unsigned char method;
	char *name;
} func;

typedef struct forStruct {
	unsigned test;
	unsigned enter;
	booleanlist *truelist;
	booleanlist *falselist;
} forStruct;

typedef struct contElem {
	unsigned quad;
	struct contElem *next;
} contElem;

typedef struct breakElem {
	unsigned quad;
	struct breakElem *next;
} breakElem;

typedef struct stmtStruct {
	contElem *contlist;
	breakElem *breaklist;
	struct stmtStruct *next;
} stmtStruct;

typedef struct quad {
	iopcode op;
	expr *result;
	expr *arg1;
	expr *arg2;
	unsigned label;
	unsigned line;
	unsigned taddress;
} quad;

extern quad *	Quads;
extern unsigned total_i;
extern unsigned currQuad;
extern unsigned programVarOffset;
extern unsigned functionLocalOffset;
extern unsigned formalArgOffset;
extern unsigned scopeSpaceCounter;

scopespace_t currscopespace(void);
unsigned currscopeoffset(void);
void inccurrscopeoffset(void);

void expand_i (void);

char *newtempfuncname(void);
char *newtempname(void);
SymbolTableEntry *newtemp(void);
void resettemp(void);
unsigned istempname(char *s);
unsigned istempexpr(expr* e);

void emit_i(iopcode op,expr *arg1,expr *arg2,expr *result,unsigned label,unsigned line);
expr *emit_iftableitem (expr *e);
expr *make_call(expr *lvalue, expr *elist, int line);

expr *newexpr(expr_t t);
expr *newexpr_conststring(char *s);
expr *newexpr_constbool(unsigned char b);
expr *newexpr_constnum(double d);

expr *member_item(expr *lvalue,char *name);
expr *lvalue_expr(SymbolTableEntry *sym);

void restorecurrscopeoffset(unsigned n);
void patchlabel(unsigned quadNo, unsigned label);

void checkuminus(expr *e);
void checkoperands(expr *e);

stmtStruct * deletestmt(stmtStruct *head);
booleanlist * makelist(unsigned quad);
booleanlist * merge(booleanlist *h1, booleanlist *h2);
void backpatch (booleanlist *head, int label);

expr * emit_operation(expr* arg1, expr* arg2, iopcode op, int line);

void printQuads();
void convertExpression(expr *expr);

void printbooleanlist(booleanlist *);

#endif