#include "avmCode.h"

// #define DEBUG_PRINTS

avm_memcell stack[AVM_STACKSIZE];
avm_memcell ax, bx;
avm_memcell retval;

unsigned 		top, topsp, totalActuals = 0;
unsigned 		executionFinished = 0;
unsigned 		pc = 0;
unsigned		currLine = 0;
char * 			typeStrings[] = {"number","string","bool","table","userfunc","libfunc","nil","undef"};

static int 		numberOfGlobals;

memclear_func_t memclearFuncs[] = {
	0, /* number */
	memclear_string,
	0, /* bool */
	memclear_table,
	0, /* usrfunc */
	0, /* libfunc*/
	0, /* nil */
	0, /* undef */
};

execute_func_t executeFuncs[]= {
	execute_assign,
	execute_add,
	execute_sub,
	execute_mul,
	execute_div,
	execute_mod,
	execute_jeq,
	execute_jne,
	execute_jle,
	execute_jge,
	execute_jlt,
	execute_jgt,
	execute_call,
	execute_pusharg,
	0,
	0,
	execute_funcenter,
	execute_funcexit,
	execute_newtable,
	execute_tablegetelem,
	execute_tablesetelem,
	execute_jump,
	execute_nop
};

library_func_t libfunccall[]={
   libfunc_print,
   libfunc_input,
   libfunc_objectmemberkeys,
   libfunc_objecttotalmembers,
   libfunc_objectcopy,
   libfunc_totalarguments,
   libfunc_argument,
   libfunc_typeof,
   libfunc_strtonum,
   libfunc_sqrt,
   libfunc_cos,
   libfunc_sin
};

arithmetic_func_t arithmeticFuncs[] = {
    add_impl,
    sub_impl,
    mul_impl,
    div_impl,
    mod_impl
};

cmp_func_t comparisonFuncs[] = {
	jle_impl,
	jge_impl,
	jlt_impl,
	jgt_impl
};

tobool_func_t toboolFuncs[] = {
    number_tobool,
    string_tobool,
    bool_tobool,
    table_tobool,
    userfunc_tobool,
    libfunc_tobool,
    nil_tobool,
    undef_tobool
};

tostring_func_t tostringFuncs[] = {
    number_tostring,
    string_tostring,
    bool_tostring,
    table_tostring,
    userfunc_tostring,
    libfunc_tostring,
    nil_tostring,
    undef_tostring
};

tonum_func_t tonumFuncs[] = {
    number_tonum,
    string_tonum,
    bool_tonum,
    table_tonum,
    userfunc_tonum,
    libfunc_tonum,
    nil_tonum,
    undef_tonum
};

static void avm_initstack(){
	unsigned i;
	for(i=0; i< AVM_STACKSIZE; ++i){
		AVM_WIPEOUT(stack[i]);
		stack[i].type = undef_m;
	}
}

avm_table* avm_tablenew(){
	avm_table *t = (avm_table*) malloc (sizeof(avm_table));
	AVM_WIPEOUT(*t);

	t->refCounter = t->total =0;
	avm_tablebucketsinit(t->numIndexed);
	avm_tablebucketsinit(t->strIndexed);

	return t;
}

void avm_tablebucketsinit(avm_table_bucket **p){
	unsigned i;
	for(i=0; i<AVM_TABLE_HASHSIZE; ++i){
		p[i] = (avm_table_bucket *) 0;
	}
}

void avm_tableincrefcounter(avm_table *t) { ++t->refCounter; }

void avm_tabledecrefcounter(avm_table *t){
	assert(t->refCounter > 0);
	if (!--t->refCounter)
		avm_tabledestroy(t);
}

void avm_tablebucketsdestroy(avm_table_bucket **p){
	unsigned i;
	for(i=0; i< AVM_TABLE_HASHSIZE; ++i){
		avm_table_bucket *b;
		for(b = *p; b;){
			avm_table_bucket *del = b;
			b = b->next;
			avm_memcellclear(&del->key);
			avm_memcellclear(&del->value);
			free(del);
		}
		p[i] = (avm_table_bucket *) 0;
	}
}

void avm_tabledestroy(avm_table *t){
	avm_tablebucketsdestroy(t->strIndexed);
	avm_tablebucketsdestroy(t->numIndexed);
	free(t);
}

void avm_initialize (void) {
	avm_initstack();
	avm_registerlibfunc("print", libfunc_print);
	avm_registerlibfunc("typeof", libfunc_typeof);
	avm_registerlibfunc("input", libfunc_input);
	avm_registerlibfunc("objectmemberkeys", libfunc_objectmemberkeys);
	avm_registerlibfunc("objecttotalmembers", libfunc_objecttotalmembers);
	avm_registerlibfunc("objectcopy", libfunc_objectcopy);
	avm_registerlibfunc("totalarguments", libfunc_totalarguments);
	avm_registerlibfunc("argument", libfunc_argument);
	avm_registerlibfunc("strtonum", libfunc_strtonum);
	avm_registerlibfunc("sqrt", libfunc_sqrt);
	avm_registerlibfunc("cos", libfunc_cos);
	avm_registerlibfunc("sin", libfunc_sin);
}

avm_memcell* avm_translate_operand(vmarg *arg, avm_memcell* reg){
	switch(arg->type){
		case global_a: 	return &stack[AVM_STACKSIZE-1-arg->val];
		case local_a:	return &stack[topsp-arg->val];
		case formal_a:	return &stack[topsp+AVM_STACKENV_SIZE+1+arg->val];
		case retval_a:	return &retval;
		case number_a:{
			reg->type = number_m;
			reg->data.numVal = consts_getnumber(arg->val);
			return reg;
		}
		case string_a:{
			reg->type = string_m;
			reg->data.strVal = consts_getstring(arg->val);
			return reg;
		}
		case bool_a:{
			reg->type = bool_m;
			reg->data.boolVal = arg->val;
			return reg;
		}
		case nil_a:{
			reg->type = nil_m;
			return reg;
		}
		case userfunc_a:{
			reg->type = userfunc_m;
			reg->data.funcVal = userfunc_getaddress(arg->val);
			return reg;
		}
		case libfunc_a: {
			reg->type = libfunc_m;
			reg->data.libfuncVal = libfuncs_getused(arg->val);
			return reg;
		}
		default:{
			avm_error("wrong type %d!\n",arg->type);
		}
	}
}

void avm_memcellclear(avm_memcell *m){
	if(m->type != undef_m){
		memclear_func_t f = memclearFuncs[m->type];
		if(f)
			(*f)(m);
		m->type = undef_m;
	}
}

void avm_assign(avm_memcell *lv, avm_memcell *rv){
	
	if(lv == rv)
		return;

	if(lv->type == table_m &&
		rv->type == table_m &&
		lv->data.tableVal == rv->data.tableVal)
		return;

	if(rv->type == undef_m)
		avm_error("assigning from 'undef' content!\n");

	avm_memcellclear(lv);

	memcpy(lv, rv, sizeof(avm_memcell));

	if(lv->type == string_m)
		lv->data.strVal = strdup(rv->data.strVal);
	else
	if(lv->type == table_m)
		avm_tableincrefcounter(lv->data.tableVal);
}

void avm_dec_top(void){
	if(!top){
		avm_error("stack overflow!\n");
	} else {
		--top;
	}
}

void avm_push_envvalue(unsigned val){
	stack[top].type = number_m;
	stack[top].data.numVal = val;
	avm_dec_top();
}

void avm_callsaveenvironment(void){
	avm_push_envvalue(totalActuals);
	avm_push_envvalue(pc+1);
	avm_push_envvalue(top + totalActuals + 2);
	avm_push_envvalue(topsp);
}

unsigned avm_get_envvalue(unsigned i){
	assert(stack[i].type == number_m);
	unsigned val = (unsigned) stack[i].data.numVal;
	assert(stack[i].data.numVal == ((double) val));
	return val;
}

void avm_calllibfunc(char *id){
	library_func_t f = avm_getlibraryfunc(id);
	if(!f){
		avm_error("unsupported lib func '%s' called!\n", id);
	} else {
		topsp = top;
		totalActuals = 0;
		(*f)();
		if(!executionFinished)
			execute_funcexit((instruction *) 0);
	}
}

unsigned avm_totalactuals (void){
  	return avm_get_envvalue(topsp + AVM_NUMACTUALS_OFFSET);
}

avm_memcell* avm_getactual (unsigned i){
	assert (i < avm_totalactuals());
	return &stack[topsp +AVM_STACKENV_SIZE + 1 +i];
}

void avm_registerlibfunc (char* id, library_func_t addr) {}

void libfunc_print (void){
	unsigned i;
	unsigned n = avm_totalactuals();
	for (i=0; i<n; ++i){
		char* s = strdup(avm_tostring(avm_getactual(i)));
		printf("%s",s);
		free(s);
	}
	printf("\n");
}
void libfunc_typeof(void){
    unsigned n = avm_totalactuals();
    if(n != 1){
		avm_error("one argument (not %d) expected in 'typeof'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		retval.type = string_m;
		retval.data.strVal = strdup(typeStrings[avm_getactual(0)->type]);
    }
}

void libfunc_input(void){
	assert(!"libfunc_input not implemented!\n");
}
void libfunc_objectmemberkeys(void){
	assert(!"libfunc_objectmemberkeys not implemented!\n");
}
void libfunc_objecttotalmembers(void){
	unsigned n = avm_totalactuals();
	if(n != 1){
		avm_error("one argument (not %d) expected in 'strtonum'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		retval.type = number_m;
		retval.data.numVal = avm_getactual(0)->data.tableVal->total;
	}
}
void libfunc_objectcopy(void){
	assert(!"libfunc_objectcopy not implemented!\n");
}
void libfunc_argument(void){
	assert(!"libfunc_argument not implemented!\n");
}
void libfunc_strtonum(void){
	unsigned n = avm_totalactuals();
	char * tmp;
	if(n != 1){
		avm_error("one argument (not %d) expected in 'strtonum'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		retval.type = number_m;
		retval.data.numVal = avm_tonum(avm_getactual(0));
	}
}
void libfunc_sqrt(void){
	unsigned i;
	unsigned n = avm_totalactuals();
	if(n != 1){
		avm_error("one arguments (not %d) expected in 'sqrt'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		unsigned rst = avm_tonum(avm_getactual(0));		
		retval.type = number_m;
		retval.data.numVal = sqrt(rst);
	}
}
void libfunc_cos(void){
	unsigned n = avm_totalactuals();
	if(n != 1){
		avm_error("one argument (not %d) expected in 'cos'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		unsigned rst = avm_tonum(avm_getactual(0));
		retval.type = number_m;
		retval.data.numVal = cos(rst);
	}
}
void libfunc_sin(void){
	unsigned n = avm_totalactuals();
	if(n != 1){
		avm_error("one argument (not %d) expected in 'sin'!\n",n);
    } else {
		avm_memcellclear(&(retval));
		unsigned rst = avm_tonum(avm_getactual(0));
		retval.type = number_m;
		retval.data.numVal = sin(rst);
	}
}

unsigned string_to_number(char * str){
	char *tmp;
	unsigned result = 0, puiss = 1;
	for(tmp = str; *tmp; tmp++){
		if(*tmp == '\"') continue;
		if( (*tmp == '-' ) || (*tmp == '+')){
			printf("%c",*tmp);
			if(*tmp == '-') puiss *= -1;
		}
		if( (*tmp >= '0') && (*tmp <= '9')){
			printf("%c",*tmp);
			result = (result * 10) + (*tmp - '0');
		}
	}
	return (result*puiss);
}

void libfunc_totalarguments (void) {
	unsigned p_topsp = avm_get_envvalue (topsp + AVM_SAVEDTOPSP_OFFSET);
	avm_memcellclear(&retval);

	if (!p_topsp) {
		avm_error ("'totalarguments' called outside of function!\n");
		retval.type = nil_m;
	}
	else {
		retval.type = number_m;
		retval.data.numVal = avm_get_envvalue (p_topsp + AVM_NUMACTUALS_OFFSET);
	}
}

double consts_getnumber(unsigned index){
	assert(totalNumConsts > index);
	numElem *tmp;
	unsigned i;
	for (tmp = numConsts, i=0; tmp; tmp=tmp->next, i++){
		if (index == i) return tmp->num;
	}
	return -1337;
}

char* consts_getstring(unsigned index){
	assert(totalStringConsts > index);
	stringElem *tmp;
	unsigned i;
	for (tmp  = stringConsts, i=0; tmp; tmp = tmp->next, i++){
		if (index == i) return tmp->s;
	}
	return NULL;
}

unsigned userfunc_getaddress(unsigned index){
	assert(totalUserFuncs > index);
	userfunc *tmp;
	unsigned i;
	for (tmp = userFuncs, i = 0; tmp; tmp = tmp->next, i++){
		if (index == i) return tmp->address;
	}
	return -1;
}

char* libfuncs_getused(unsigned index){
	assert(totalNamedLibfuncs > index);
	stringElem *tmp;
	unsigned i;
	for (tmp = namedLibfuncs, i = 0; tmp; tmp = tmp->next, i++){
		if (index == i) return tmp->s;
	}
	return NULL;
}

unsigned char jge_impl(double x, double y) { return x>=y; }
unsigned char jgt_impl(double x, double y) { return x>y;  }
unsigned char jle_impl(double x, double y) { return x<=y; }
unsigned char jlt_impl(double x, double y) { return x<y;  }

#ifdef DEBUG_PRINTS
static char * debug_opcodes[] = {
	"assign_v", 		"add_v",			"sub_v",
	"mul_v",			"div_v",			"mod_v",
	"jeq_v",			"jne_v",			"jle_v",
	"jge_v",			"jlt_v",			"jgt_v",
	"call_v",			"pusharg_v",		"getretval_v",
	"return_v",			"funcenter_v",		"funcexit_v",		
	"newtable_v",		"tablegetelem_v",	"tablesetelem_v",	
	"jump_v", 			"nop_v"
};
#endif

unsigned execute_cycle(){
	if(executionFinished){
		return 0;
	} else {
		if(pc >= AVM_ENDING_PC){
			executionFinished = 1;
			return 1;
		} else {
			assert(pc < AVM_ENDING_PC);
			instruction *i = Instructions + pc;

			#ifdef DEBUG_PRINTS
			printf("Current PC := %d\n", pc);
			printf("Opcode := %s\n",debug_opcodes[i->opcode]);
			#endif

			assert(i->opcode >= 0 && i->opcode <= AVM_MAX_INSTRUCTIONS);
			if(i->srcLine) currLine = i->srcLine;
			unsigned oldPc = pc;
			(*executeFuncs[i->opcode])(i);
			if(pc == oldPc){
				++pc;
			}
			return 1;
		}
	}
	return 0;
}

void execute_assign(instruction *instr){
	avm_memcell *lv = avm_translate_operand(&instr->result, (avm_memcell *) 0);
	avm_memcell *rv = avm_translate_operand(&instr->arg1, &ax);
	assert(lv && ( &stack[AVM_STACKSIZE-1] >= lv && lv > &stack[top] || lv == &retval));
	assert(rv);
	avm_assign(lv,rv);
}

void execute_call(instruction *instr){
	avm_memcell *func = avm_translate_operand(&instr->result, &ax);
	assert(func);
	avm_callsaveenvironment();
	switch(func->type){
		case userfunc_m:{
			pc = func->data.funcVal;
			assert(pc < AVM_ENDING_PC);
			printf("Instructions[%d].opcode = %d\n", pc, Instructions[pc].opcode);
			assert(Instructions[pc].opcode == funcenter_v);
			break;
		}
		case string_m: {
			avm_calllibfunc(func->data.strVal); 
			break;
		}
		case libfunc_m: {
			avm_calllibfunc(func->data.libfuncVal); 
			break;
		}

		default: {
			char *s = avm_tostring(func);
			avm_error("call: cannot bind '%s' to function!\n", s);
			free(s);
		}
	}
}

void execute_funcenter(instruction *instr){
	avm_memcell *func = avm_translate_operand(&instr->result, &ax);
	assert(func);
	assert(pc == func->data.funcVal);

	totalActuals = 0;
	userfunc* funcInfo = avm_getfuncinfo(pc);
	topsp = top;
	top = top - funcInfo->localSize;
}

void execute_funcexit(instruction *instr){
	unsigned oldTop = top;
	top = avm_get_envvalue(topsp + AVM_SAVEDTOP_OFFSET);
	pc = avm_get_envvalue(topsp + AVM_SAVEDPC_OFFSET);
	topsp = avm_get_envvalue(topsp + AVM_SAVEDTOPSP_OFFSET);
	while(oldTop++ <= top)
		avm_memcellclear(&stack[oldTop]);
}

void execute_pusharg(instruction *instr){
	avm_memcell* arg = avm_translate_operand(&instr->result,&ax);
	assert(arg);
	avm_assign(&stack[top],arg);
	++totalActuals;
	avm_dec_top();
}

void execute_arithmetic(instruction * instr){
    avm_memcell * lv = avm_translate_operand(&(instr->result),(avm_memcell *) 0);
    avm_memcell * rv1 = avm_translate_operand(&(instr->arg1),&ax);
    avm_memcell * rv2 = avm_translate_operand(&(instr->arg2),&bx);
	assert(lv);
	assert(rv1);
	assert(rv2);
	if(rv1->type != number_m || rv2->type != number_m){
		avm_error("not a number in arithmetic!\n");
	} else {
	    arithmetic_func_t op = arithmeticFuncs[instr->opcode - add_v];
	    avm_memcellclear(lv);
	    lv->type = number_m;
	    lv->data.numVal = (*op)(rv1->data.numVal,rv2->data.numVal); 
	}
}

void execute_jeq(instruction *instr){
	assert(instr->result.type == label_a);
   	avm_memcell * rv1 = avm_translate_operand(&(instr->arg1),&ax);
   	avm_memcell * rv2 = avm_translate_operand(&(instr->arg2),&bx);
   	unsigned char result = 0;

    if(rv1->type == undef_m || rv2->type == undef_m){
        avm_error("'undef' involved in equality!\n");
    }
    else if(rv1->type == nil_m || rv2->type == nil_m){
	    result = rv1->type == nil_m && rv2->type == nil_m;
	}
	else if(rv1->type== bool_m || rv2->type == bool_m ){
	      result = (avm_tobool(rv1) == avm_tobool(rv2));
	}
	else if(rv1->type != rv2->type){
	    avm_error("%s == %s is illegal!\n",typeStrings[rv1->type],typeStrings[rv2->type]);
	} else {
		if(rv1->type == string_m){
		    if(strcmp(rv1->data.strVal,rv2->data.strVal) == 0){
				result = 1;
			}
		}
		else if(rv1->type == libfunc_m){
			if(strcmp(rv1->data.libfuncVal,rv2->data.libfuncVal) == 0){
				result = 1;
			}
		}
		else if(rv1->type == userfunc_m){
			if(rv1->data.funcVal == rv2->data.funcVal){
				result = 1;
			}
		}
		else if(rv1->type == number_m){
			if(rv1->data.numVal == rv2->data.numVal){
				result = 1;
			}
		}
		else if(rv1->type == table_m){
			if(rv1->data.tableVal == rv1->data.tableVal){
				result = 1;
			}
		}
		else{
			avm_error("illegal type!\n");
		}  
	}
    if(!executionFinished && result)
		pc = instr->result.val;
}

void execute_jne(instruction *instr){
	assert(instr->result.type == label_a);
   	avm_memcell * rv1 = avm_translate_operand(&(instr->arg1),&ax);
   	avm_memcell * rv2 = avm_translate_operand(&(instr->arg2),&bx);
   	unsigned char result = 0;
    if(rv1->type == undef_m || rv2->type == undef_m){
        avm_error("'undef' involved in equality!\n");
    }
    else if(rv1->type == nil_m || rv2->type == nil_m){
	    result = rv1->type == nil_m && rv2->type == nil_m;
	}
	else if(rv1->type== bool_m || rv2->type == bool_m ){
	      result = (avm_tobool(rv1) == avm_tobool(rv2));
	}
	else if(rv1->type != rv2->type){
	    avm_error("%s == %s is illegal!\n",typeStrings[rv1->type],typeStrings[rv2->type]);
	} else {
		if(rv1->type == string_m){
		    if(strcmp(rv1->data.strVal,rv2->data.strVal) == 0){
				result = 1;
			}
		}
		else if(rv1->type == libfunc_m){
			if(strcmp(rv1->data.libfuncVal,rv2->data.libfuncVal) == 0){
				result = 1;
			}
		}
		else if(rv1->type == userfunc_m){
			if(rv1->data.funcVal == rv2->data.funcVal){
				result = 1;
			}
		}
		else if(rv1->type == number_m){
			if(rv1->data.numVal == rv2->data.numVal){
				result = 1;
			}
		}
		else if(rv1->type == table_m){
			if(rv1->data.tableVal == rv1->data.tableVal){
				result = 1;
			}
		}
		else{
			avm_error("illegal type!\n");
		}  
	}
    if(!executionFinished && !result)
		pc = instr->result.val;
}

void execute_comparison(instruction* instr) {
	assert(instr->result.type == label_a);
   	avm_memcell * rv1 = avm_translate_operand(&(instr->arg1),&ax);
   	avm_memcell * rv2 = avm_translate_operand(&(instr->arg2),&bx);
   	unsigned char result = 0;
    if(rv1->type == undef_m || rv2->type == undef_m){
        avm_error("'undef' involved in equality!\n");
    }
    else if(rv1->type == nil_m || rv2->type == nil_m){
	    result = rv1->type == nil_m && rv2->type == nil_m;
	}
	else if(rv1->type== bool_m || rv2->type == bool_m ){
	      result = (avm_tobool(rv1) == avm_tobool(rv2));
	}
	else if(rv1->type != rv2->type){
	    avm_error("%s == %s is illegal!\n",typeStrings[rv1->type],typeStrings[rv2->type]);
	} else {
		if(rv1->type == number_m && rv2->type == number_m){
			if((* comparisonFuncs[instr->opcode - jle_v])(rv1->data.numVal,rv2->data.numVal)){
				pc = instr->result.val;
			}
		}
	}		
}

void execute_tablesetelem(instruction* instr) {
	avm_memcell* t = avm_translate_operand(&instr->result, (avm_memcell*) 0);
	avm_memcell* c = avm_translate_operand(&instr->arg1, &ax);
	avm_memcell* i = avm_translate_operand(&instr->arg2, &bx);
	assert (t && (&stack[AVM_STACKSIZE-1] >= t && t > &stack[top]));
	assert (i && c);
	if (t->type != table_m){
		avm_error ("illegal use of type %s as table!\n", typeStrings[t->type]);
	} else {
		avm_tablesetelem(t->data.tableVal, i, c);
	}
}

void execute_tablegetelem(instruction* instr) {
	avm_memcell* lv = avm_translate_operand(&instr->result, (avm_memcell*) 0);
	avm_memcell* t = avm_translate_operand(&instr->arg1, (avm_memcell*) 0);
	avm_memcell* i = avm_translate_operand(&instr->arg2, &ax);
	assert(lv && (&stack[AVM_STACKSIZE-1] >= lv) && lv >&stack[top] || lv == &retval);
	assert(t && (&stack[AVM_STACKSIZE-1] >= t) && t >&stack[top]);
	assert(i);
	avm_memcellclear(lv);
	lv->type = nil_m;
	if (t->type != table_m) {
		avm_error ("illegal use of type %s as table!\n", typeStrings[t->type] );
	} else {
		avm_memcell* content = avm_tablegetelem(t->data.tableVal, i);
		if (content){
			avm_assign (lv, content);
		} else {
			char * ts = avm_tostring(t);
			char * is = avm_tostring(i);
			avm_warning ("%s[%s] not found!", ts, is);
			free(ts);
			free(is);
		}

	}
}

void execute_newtable(instruction* instr) {
	avm_memcell* lv = avm_translate_operand(&instr -> result, NULL);
	assert (lv && (&stack[AVM_STACKSIZE-1] >= lv && lv > &stack[top]) || lv == &retval);
	avm_memcellclear (lv);
	lv->type = table_m;
	lv->data.tableVal = avm_tablenew();
	avm_tableincrefcounter(lv ->data.tableVal);
}

void execute_jump(instruction *instr){
	pc = instr->result.val;
}

void execute_nop(instruction* instr) {}

double add_impl (double x,double y) { return x+y; }
double sub_impl (double x,double y) { return x-y; }
double mul_impl (double x,double y) { return x*y; }
double div_impl (double x,double y) {
	if(y){
		return x/y;
	} else {
		avm_error("cant divide with zero!\n");
		return -1;
	}
}
double mod_impl (double x,double y) {
	if(y){
		return (unsigned) x% (unsigned) y;
	} else {
		avm_error("cant mod with zero!\n");
		return -1;
  }
}

char* number_tostring(avm_memcell *m){
    char *s = malloc (100);
    sprintf(s, "%lf", m->data.numVal);
    return s;
}
unsigned char number_tobool(avm_memcell * m){ 
	return (m->data.numVal != 0); 
}
unsigned number_tonum(avm_memcell *m){
	return (m->data.numVal);
}

char* string_tostring(avm_memcell* m){
    return strdup(m->data.strVal);
}
unsigned char string_tobool(avm_memcell * m){ 
	return (m->data.strVal[0] != 0); 
}
unsigned string_tonum(avm_memcell *m){
	return string_to_number(m->data.strVal);
}

char* bool_tostring(avm_memcell* m){
	if(avm_tobool(m)){
		return "TRUE";
	}else{
		return "FALSE";
	}
	assert(0);
}
unsigned char bool_tobool(avm_memcell * m){ 
	return (m->data.boolVal); 
}
unsigned bool_tonum(avm_memcell *m){
	if(avm_tobool(m)){
		return 1;
	}else{
		return 0;
	}
}

char* table_tostring(avm_memcell* m){
    assert(!"table to string not implemented!\n");
}
unsigned char table_tobool(avm_memcell * m){ 
	return 1; 
}
unsigned table_tonum(avm_memcell *m){
	return 1;
}

char* userfunc_tostring(avm_memcell *m){
    userfunc *f;
    f = avm_getfuncinfo(m->data.funcVal);
    return f->id;
}
unsigned char userfunc_tobool(avm_memcell * m){ 
	return 1; 
}
unsigned userfunc_tonum(avm_memcell *m){
	return 1;
}

char* libfunc_tostring(avm_memcell* m){
    return strdup(m->data.libfuncVal);
}
unsigned char libfunc_tobool(avm_memcell * m){ 
	return 1; 
}
unsigned libfunc_tonum(avm_memcell *m){
	return 1;
}

char* nil_tostring(avm_memcell *m){
    return "NIL";
}
unsigned char nil_tobool(avm_memcell * m){ 
	return 0;
}
unsigned nil_tonum(avm_memcell *m){
	return 0;
}

char* undef_tostring(avm_memcell *m){
    return "UNDEF";
}
unsigned char undef_tobool(avm_memcell * m){ 
	return 0;
}
unsigned undef_tonum(avm_memcell *m){
	return 0;
}

char* avm_tostring(avm_memcell * m) {
	assert(m->type >= number_m && m->type <= undef_m);
	return (*tostringFuncs[m->type])(m);
}
unsigned char avm_tobool (avm_memcell *m){
	assert(m->type >= 0 && m->type < undef_m);
	return (*toboolFuncs[m->type])(m);
}
unsigned avm_tonum(avm_memcell * m) {
	assert(m->type >= number_m && m->type <= undef_m);
	return (*tonumFuncs[m->type])(m);
}

static char * libNames[] = { 
	"print",
	"input",
	"objectmemberkeys",
	"objecttotalmembers",
	"objectcopy",
	"totalarguments",
	"argument",
	"typeof",
	"strtonum",
	"sqrt",
	"cos",
	"sin"
};

library_func_t avm_getlibraryfunc (char* id){
	unsigned i;
	for(i = 0; i < 12;i++){
		if(!strcmp(libNames[i],id)){
			return libfunccall[i];
		}
	}
 	avm_error("unsupported lib func '%s' called!\n",id);	
}

userfunc *avm_getfuncinfo(unsigned address){
	userfunc *tmp;
	for (tmp = userFuncs; tmp; tmp=tmp->next){
		if (tmp->address == address){
			return tmp;
		}
	}
	avm_error("unexistent userfunc address!\n");
}

unsigned avm_hash(avm_memcell *i){
	unsigned hash = 0;
	char *p;
	if(i->type == number_m){
		hash = (int)i->data.numVal % AVM_TABLE_HASHSIZE;
	}
	else if(i->type == string_m){
		p = i->data.strVal;
		int size = strlen(p), cc;
		for (cc = 0; cc < size; cc++){
			hash = 666*hash + p[cc];
		}
	} else {
		avm_error("wrong type at hash function!\n");
	}
	return hash;
}

avm_memcell* avm_tablegetelem (avm_table* table, avm_memcell* index){
	unsigned i;
	avm_table_bucket *p;
	if (index->type == nil_m || index->type == undef_m || index->type == table_m){
		avm_error("invalid table index!\n");
	}
	i = avm_hash(index);
	assert(i >= 0);
	switch(index->type){
		case(number_m):{
			p = table->numIndexed[i];
			for(p = table->numIndexed[i]; p; p = p->next){
				if (p->key.data.numVal == index->data.numVal) return &p->value;
			}
			break;
		}
		case(string_m):{
			for(p = table->strIndexed[i]; p; p = p->next){
				if (strcmp(p->key.data.strVal, index->data.strVal) == 0) return &p->value;
			}
			break;
		}
		default:{ 
			avm_error("invalid index type!\n");
		}
	}
	return NULL;
}

void avm_tablesetelem (avm_table* table, avm_memcell* index, avm_memcell* content){
	avm_table_bucket *new, *ptr, *temp = NULL;
	instruction *t;
	t = (instruction *) &Instructions[pc];
	unsigned i;
	if (index->type == nil_m || index->type == undef_m || index->type == table_m){
		avm_error("invalid table index!\n");
		return;
	}
	i = avm_hash(index);
	assert(i >= 0);
	switch(index->type){
		case(number_m):{
			ptr = table->numIndexed[i];
			for(ptr = table->numIndexed[i]; ptr; ptr = ptr->next){
				if (ptr->key.data.numVal == index->data.numVal){
					if (content->type == nil_m){
						if (temp == NULL) table->numIndexed[i] = ptr->next;
						else temp->next = ptr->next;
						avm_memcellclear(&ptr->value);
						avm_memcellclear(&ptr->key);
						return;
					} else {
						avm_memcellclear(&ptr->value);
						avm_assign(&ptr->value, content);
						table->total++;
						return;
					}
				}
				temp = ptr;
			}
			if (content->type != nil_m){
				new = (avm_table_bucket *) malloc(sizeof(avm_table_bucket));
				new->value.type = undef_m;
				new->key.type = undef_m;
				if (temp == NULL) table->numIndexed[i] = new;
				else temp->next = new;
				new->next = NULL;
				avm_assign(&new->key, index);
				avm_assign(&new->value, content);
				table->total++;
			}
			break;
		}
		case(string_m):{
			ptr = table->strIndexed[i];
			for (ptr = table->strIndexed[i]; ptr; ptr = ptr->next){
				if (strcmp(ptr->key.data.strVal, index->data.strVal) == 0){
					if (content->type == nil_m){
						if (temp == NULL) table->strIndexed[i] = ptr->next;
						else temp->next = ptr->next;
						avm_memcellclear(&ptr->value);
						avm_memcellclear(&ptr->key);
						return;
					} else {
						avm_memcellclear(&ptr->value);
						avm_assign(&ptr->value, content);
						table->total++;
						return;
					}
				}
				temp = ptr;
			}
			if (content->type != nil_m){
				new = (avm_table_bucket *) malloc(sizeof(avm_table_bucket));
				new->key.type = undef_m;
				new->value.type = undef_m;
				if (temp == NULL) table->strIndexed[i] = new;
				else temp->next = new;
				new->next = NULL;
				avm_assign(&new->key, index);
				avm_assign(&new->value, content);
				table->total++;
			}
			break;
		}
		default:{
			assert(0);
			return;
		}
	}
}

void read_from_binary(){
	FILE * fp;
	int i,length, size, magic_num;
	char *tmp = malloc(200*sizeof(char));
	double num;
	userfunc *ftmp;
	instruction *t;
	fp = fopen("TargetFile.abc","rb+");

	fread(&magic_num,sizeof(int),1,fp);
	assert(magic_num == MagicNumber);
	fread(&numberOfGlobals,sizeof(int),1,fp);
	fread(&totalStringConsts,sizeof(int),1,fp);
	fread(&totalNumConsts,sizeof(int),1,fp);
	fread(&totalUserFuncs,sizeof(int),1,fp);
	fread(&totalNamedLibfuncs,sizeof(int),1,fp);
	fread(&size,sizeof(int),1,fp);

	for(i=0; i<totalStringConsts; i++){
		fread(&length,sizeof(int),1,fp);
		fread(tmp,sizeof(char),length,fp);
		LIST_INSERT(stringConsts, STRINGSTATEMENT(tmp), stringElem, ADD_2_BACK);
		memset(tmp,0,length);
	}
	for(i=0; i<totalNumConsts;i++){
		fread(&num,sizeof(double),1,fp);
		LIST_INSERT(numConsts, NUMBERSTATEMENT(num), numElem, ADD_2_BACK);
	}
	for(i=0; i<totalUserFuncs; i++){
		ftmp = (userfunc *) malloc(sizeof(userfunc));
		fread(ftmp,sizeof(userfunc),1,fp);
		fread(&length,sizeof(int),1,fp);
		fread(tmp,sizeof(char),length,fp);
		ftmp->id = strdup(tmp);
		LIST_INSERT(userFuncs, FUNCTIONSTATEMENT(ftmp->address, ftmp->localSize, ftmp->id), userfunc, ADD_2_BACK);
		memset(tmp,0,length);
	}
	for(i=0; i<totalNamedLibfuncs; i++){
		fread(&length,sizeof(int),1,fp);
		fread(tmp,sizeof(char),length,fp);
		LIST_INSERT(namedLibfuncs, STRINGSTATEMENT(tmp), stringElem, ADD_2_BACK);
		memset(tmp,0,length);
	}
	for(i=0; i<size; i++){
		t = malloc(sizeof(struct instruction));
		fread(t,sizeof(instruction),1,fp);
		emit_t(t);
	}
	fclose(fp);
}

int main(void){
	read_from_binary();

	#ifdef DEBUG_PRINTS
	printInstructions();
	#endif

	top = AVM_STACKSIZE-1-numberOfGlobals;
	topsp = top;
	avm_initialize();
	while (execute_cycle());
	return 1;
}
