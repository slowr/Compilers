#include "intermediateCode.h"


unsigned 	total_i = 0;
unsigned 	currQuad = 0;
unsigned 	programVarOffset = 0;
unsigned 	functionLocalOffset = 0;
unsigned 	formalArgOffset = 0;
unsigned 	scopeSpaceCounter = 1;
unsigned 	tempcounter = 0;
unsigned 	tempfuncounter = 0;

quad *		Quads = NULL;

scopespace_t currscopespace(){
	if (scopeSpaceCounter == 1)
		return programvar;
	else
	if (scopeSpaceCounter % 2 == 0)
		return formalarg;
	else 
		return functionlocal;
}

unsigned currscopeoffset(){
	switch (currscopespace()){
		case programvar : return programVarOffset;
		case functionlocal : return functionLocalOffset;
		case formalarg : return formalArgOffset;
		default: assert(0);
	}
}

void inccurrscopeoffset(){
	switch (currscopespace()){
		case programvar : ++programVarOffset; break;
		case functionlocal : ++functionLocalOffset; break;
		case formalarg : ++formalArgOffset; break;
		default : assert(0);
	}
}

void expand_i(){
	assert(total_i == currQuad);
	quad *p = (quad*) malloc (NEW_SIZE_I);
	if (Quads) {
		memcpy(p, Quads, CURR_SIZE_I);
		free(Quads);
	}
	Quads = p;
	total_i += EXPAND_SIZE_I;
}

void emit_i(iopcode op,expr *arg1,expr *arg2,expr *result,unsigned label,unsigned line){
	if(currQuad == total_i)
		expand_i();

	quad *p = Quads+currQuad++;
	p->arg1 = arg1;
	p->arg2 = arg2;
	p->result = result;
	p->label = label;
	p->line = line;
	p->op = op;
}

char *newtempname(){
	char *tmp = (char *) malloc (10*sizeof(char));
	sprintf(tmp, "_t%d",tempcounter++);
	return tmp;
}

char *newtempfuncname(){
	char *tmp = (char *) malloc (10*sizeof(char));
	sprintf(tmp, "_f%d",tempfuncounter++);
	return tmp;
}

void resettemp(){ tempcounter = 0; }

SymbolTableEntry *newtemp(){
	SymbolTableEntry *sym = NULL;
	char *n = newtempname();
	ScopeNode *sn = lookup(scopelist, curr_scope, n);
	if(sn) sym = sn->symbol;

	if(!sym){
		sym = put(symTable, &scopelist, strdup(n), curr_scope, -1, !curr_scope ? GLOBAL_T : LOCAL_T, var_s, currscopespace());
		sym->offset = currscopeoffset();
		inccurrscopeoffset();
	}

	return sym;
}

expr *emit_iftableitem (expr *e){
	if (e->type != tableitem_e)
		return e;
	else{
		expr *result = newexpr(var_e);
		result->sym = newtemp();
		emit_i(tablegetelem, e, e->index, result, currQuad, -1);
		return result;
	}
}

expr *newexpr(expr_t t){
	expr *e = (expr *) malloc (sizeof(expr));
	memset(e, 0, sizeof(expr));
	e->type = t;
	return e;
}

expr *newexpr_conststring(char *s){
	expr *e = newexpr(conststring_e);
	e->strConst = strdup(s);
	return e;
}

expr *newexpr_constbool(unsigned char b){
	expr *e = newexpr(constbool_e);
	e->boolConst = b;
	return e;
}

expr *newexpr_constnum(double d){
	expr *e = newexpr(constnum_e);
	e->numConst = d;
	return e;
}

expr *member_item(expr *lvalue,char *name){
	lvalue = emit_iftableitem(lvalue);
	expr *item = newexpr(tableitem_e);
	item->sym = lvalue->sym;
	item->index = newexpr_conststring(name);

	return item;
}

expr *lvalue_expr(SymbolTableEntry *sym){
	assert(sym);
	expr* e = (expr *) malloc (sizeof(expr));
	memset(e, 0, sizeof(expr));

	e->next = (expr*) 0;
	e->sym = sym;

	switch(sym->symbolType){
		case var_s : e->type = var_e; break;
		case programfunc_s : e->type = programfunc_e; break;
		case libraryfunc_s : e->type = libraryfunc_e; break;
		default: assert(0);
	}

	return e;
}

expr *make_call(expr *lvalue, expr *elist, int line){
	expr *func = emit_iftableitem(lvalue);
	expr *iterator = elist;
	expr *result;
	while(elist){
		emit_i(param, NULL, NULL, elist, currQuad, line);
		elist = elist->next;
	}
	emit_i(call, NULL, NULL, func, currQuad, line);
	result = newexpr(var_e);
	result->sym = newtemp();
	emit_i(getretval, NULL, NULL, result, currQuad, line);
	return result;
}

void restorecurrscopeoffset(unsigned n){
	switch (currscopespace()){
		case programvar : programVarOffset = n; break;
		case functionlocal : functionLocalOffset = n; break;
		case formalarg : formalArgOffset = n; break;
		default : assert(0);
	}
}

void patchlabel(unsigned quadNo, unsigned label){
	assert(quadNo < currQuad);
	Quads[quadNo].label = label;
}

void checkuminus(expr *e){
	if(	e->type == constbool_e 		||
		e->type == conststring_e	||
		e->type == nil_e			||
		e->type == newtable_e		||
		e->type == programfunc_e	||
		e->type == libraryfunc_e	||
		e->type == boolexpr_e)
		fprintf(stderr,"Illegal expr to unary -");
}

void checkoperands(expr *e){
	if(	e->type == constbool_e 		||
		e->type == conststring_e	||
		e->type == nil_e			||
		e->type == programfunc_e	||
		e->type == libraryfunc_e )
		fprintf(stderr,"Illegal type to make emit_operation");
}

unsigned istempname(char *s){
	return *s == '_';
}

unsigned istempexpr(expr* e){
	return 	e->sym && 
			e->sym->type == var_s && 
			istempname(getSymName(e->sym));
}

stmtStruct * deletestmt(stmtStruct *head){
	if(head){
		stmtStruct *tmp;
		tmp = head;
		head = head->next;
		free(tmp->contlist);
		free(tmp->breaklist);
		free(tmp);
	}
	return head;
}

booleanlist * makelist(unsigned quad){
	booleanlist *newE1;
	newE1 = (booleanlist *) malloc (sizeof(booleanlist));
	newE1->value = quad;
	newE1->next = NULL;
	return newE1;
}

booleanlist * merge(booleanlist *h1, booleanlist *h2){
	if(h1){
		booleanlist *tmp = h1;
		while(tmp->next) tmp = tmp->next;
		tmp->next = h2;
		return h1;
	}
	return h2;
}

void backpatch(booleanlist *head, int label){
	booleanlist *tmp;
	for(tmp = head; tmp; tmp=tmp->next){
		patchlabel(tmp->value, label);
	}
}

expr *emit_operation(expr* arg1, expr* arg2, iopcode op, int line){
	expr *result;
	if(arg1->type == constnum_e && arg2->type == constnum_e){
		switch(op){
			case add: 	result = newexpr_constnum(arg1->numConst + arg2->numConst); break;
			case sub: 	result = newexpr_constnum(arg1->numConst - arg2->numConst); break;
			case mul: 	result = newexpr_constnum(arg1->numConst * arg2->numConst); break;
			case divi: 	result = newexpr_constnum(arg1->numConst / arg2->numConst); break;
			case mod: 	result = newexpr_constnum((int) arg1->numConst % (int)arg2->numConst); break;
			case if_greater: 	(arg1->numConst > arg2->numConst) 	? (result = newexpr_constbool(1)) : (result = newexpr_constbool(0)); break;
			case if_greatereq:	(arg1->numConst >= arg2->numConst) 	? (result = newexpr_constbool(1)) : (result = newexpr_constbool(0)); break;
			case if_less: 		(arg1->numConst < arg2->numConst) 	? (result = newexpr_constbool(1)) : (result = newexpr_constbool(0)); break;
			case if_lesseq: 	(arg1->numConst <= arg2->numConst) 	? (result = newexpr_constbool(1)) : (result = newexpr_constbool(0)); break;
			default: assert(0);
		}
	} else {
		switch(op){
			case add:
			case sub:
			case mul:
			case divi:
			case mod:
				result = newexpr(arithexpr_e);
				result->sym = newtemp();
				emit_i(op, arg1, arg2, result, currQuad, line);
				break;
			case if_greater:
			case if_greatereq:
			case if_less:
			case if_lesseq:
			case if_eq:
			case if_noteq:
				result = newexpr(boolexpr_e);
				result->truelist = makelist(NEXTQUAD);
				result->falselist = makelist(NEXTQUAD+1);
				emit_i(op, arg1, arg2, NULL, -1, line);
				emit_i(jump, NULL, NULL, NULL, -1, line);
				break;
			default: assert(0);
		}
	}
	return result;
}

void printQuads(){
	int i;
	enum iopcode x;
	quad y;
	printf("\n- [QUADS] -\n");
	for (i=0; i<currQuad; i++){
		y = Quads[i];
		x = Quads[i].op;
		printf("[%3d]  ",i);
		switch(x){
			case add:			printf("ADD "); goto twoArgsOps;
			case sub:			printf("SUB "); goto twoArgsOps;
			case mul:			printf("MUL "); goto twoArgsOps;
			case mod:			printf("MOD "); goto twoArgsOps;
			case divi:			printf("DIV "); goto twoArgsOps;
			case tablesetelem: 	printf("TABLESETELEM "); goto twoArgsOps;
			case tablegetelem: 	printf("TABLEGETELEM "); goto twoArgsOps;
				twoArgsOps:
				convertExpression(y.arg1);
				convertExpression(y.arg2);
				convertExpression(y.result);
				break;
			case if_less:		printf("IF_LESS "); goto twoArgsLabel;
			case if_lesseq:		printf("IF_LESSEQ "); goto twoArgsLabel;
			case if_greater:	printf("IF_GREATER "); goto twoArgsLabel;
			case if_greatereq:	printf("IF_GREATEREQ "); goto twoArgsLabel;
			case if_eq:			printf("IF_EQ "); goto twoArgsLabel;
			case if_noteq:		printf("IF_NOTEQ ");
				twoArgsLabel:
				convertExpression(y.arg1);
				convertExpression(y.arg2);
				printf("%d ", y.label);
				break;
			case assign:		printf("ASSIGN "); goto twoArgsBool;
				twoArgsBool:
				convertExpression(y.arg1);
				convertExpression(y.result);
				break;
			case jump:
				printf("JUMP %d ", y.label);
				break;
			case ret:
				printf("RETURN ");
				if(y.result) convertExpression(y.result);
				break;
			case param: printf("PARAM "); goto oneArg;
			case funcstart: printf("FUNCSTART "); goto oneArg;
			case funcend: printf("FUNCEND "); goto oneArg;
			case call: printf("CALL "); goto oneArg;
			case getretval: printf("GETRETVAL "); goto oneArg;
			case tablecreate: printf("TABLECREATE "); goto oneArg;
				oneArg:
				convertExpression(y.result);
				break;
			default:
				assert(0);
		}
		printf("\n");
  	}
}

void convertExpression(expr *expr){
	switch(expr->type){
		case var_e:
		case tableitem_e:
		case arithexpr_e:
		case boolexpr_e:
		case assignexpr_e:
		case newtable_e:
			printf("%s ", (expr->sym)->value.varVal->name);
			break;
		case programfunc_e:
		case libraryfunc_e:
			printf("%s ", (expr->sym)->value.funcVal->name);
			break;
		case constnum_e:
			printf("%.2f ", expr->numConst);
			break;
		case constbool_e:
			printf("%s ", expr->boolConst ? "TRUE" : "FALSE");
			break;
		case conststring_e:
			printf("%s ", expr->strConst);
			break;
		case nil_e:
			printf("NIL ");
			break;
		default:
			assert(0);
	}
}

void printbooleanlist(booleanlist *head){
	booleanlist *tmp;
	for(tmp = head ; tmp ; tmp = tmp->next){
		printf("%d ",tmp->value+1);
	}
	printf("\n");	
}
