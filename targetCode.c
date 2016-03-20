#include "targetCode.h"

unsigned 	MagicNumber = 235973201;
unsigned 	currInstruction = 0;
unsigned 	total_t = 0;
unsigned 	currProcessed = 0;
unsigned	totalNumConsts = 0;
unsigned 	totalStringConsts = 0;
unsigned 	totalNamedLibfuncs = 0;
unsigned	totalUserFuncs = 0;
unsigned	ij_total = 0;

numElem * 			numConsts = NULL;
stringElem *		stringConsts = NULL;
stringElem *		namedLibfuncs = NULL;
userfunc *			userFuncs = NULL;
instruction *		Instructions = NULL;
incomplete_jump * 	ij_head = NULL;
funcstack *			funcStack = NULL;

void (*generators[])(quad *) = {
	generate_ASSIGN,
	generate_ADD,
	generate_SUB,
	generate_MUL,
	generate_DIV,
	generate_MOD,
	generate_IF_EQ,
	generate_IF_NOTEQ,
	generate_IF_LESSEQ,
	generate_IF_GREATEREQ,
	generate_IF_LESS,
	generate_IF_GREATER,
	generate_CALL,
	generate_PARAM,
	generate_RETURN,
	generate_GETRETVAL,
	generate_FUNCSTART,
	generate_FUNCEND,
	generate_NEWTABLE,
	generate_TABLEGETELEM,
	generate_TABLESETELEM,
	generate_JUMP,
	generate_NOP
};

void generator (void) {
	unsigned i;
	for (i = 0; i < currQuad; ++i){
		(*generators[Quads[i].op])(Quads+i);
		currProcessed++;
	}
	patch_incomplete_jumps();
	WriteTargetCode();
}

void WriteTargetCode(){
	FILE *fp;
	int i, len, size;
	stringElem * Strtmp;
	numElem * Numtmp;
	userfunc * functmp;
	instruction * instrtmp;
	fp = fopen("TargetFile.abc","wb+");
	size = NEXTINSTRUCTIONLABEL;
	fwrite(&MagicNumber, sizeof(unsigned), 1, fp);
	fwrite(&programVarOffset, sizeof(int), 1, fp);
	fwrite(&totalStringConsts, sizeof(int), 1, fp);
	fwrite(&totalNumConsts, sizeof(int), 1, fp);
	fwrite(&totalUserFuncs, sizeof(int), 1, fp);
	fwrite(&totalNamedLibfuncs, sizeof(int), 1, fp);
	fwrite(&size, sizeof(int), 1, fp);

	for(Strtmp = stringConsts; Strtmp ; Strtmp=Strtmp->next){
		len = strlen(Strtmp->s);
		fwrite(&len, sizeof(int), 1, fp);
		fwrite(Strtmp->s, len*sizeof(char), 1, fp);
	}
	for(Numtmp = numConsts; Numtmp ; Numtmp=Numtmp->next){
		fwrite(&Numtmp->num, sizeof(double), 1, fp);
	}
	for(functmp = userFuncs; functmp ; functmp=functmp->next){
		len = strlen(functmp->id);
		fwrite(functmp, sizeof(userfunc), 1, fp);
		fwrite(&len, sizeof(int), 1, fp);
		fwrite(functmp->id, len*sizeof(char), 1, fp);
	}
	for(Strtmp = namedLibfuncs; Strtmp ; Strtmp=Strtmp->next){
		len = strlen(Strtmp->s);
		fwrite(&len, sizeof(int), 1, fp);
		fwrite(Strtmp->s, len*sizeof(char), 1, fp);
	}
	for(instrtmp = Instructions, i = 0; i < size; i++){
		fwrite(instrtmp+i, sizeof(instruction), 1, fp);
	}
	fclose(fp);
}

void make_operand (expr* e, vmarg* arg) {
	switch(e->type) {

		/* All those below use a variable for storage
		*/
		case var_e:
		case tableitem_e:
		case arithexpr_e:
		case boolexpr_e:
		case newtable_e:
		case assignexpr_e: {
			assert(e->sym);
			arg->val = e->sym->offset;

			switch (e->sym->scopeSpace) {
				case programvar:		arg->type = global_a; break;
				case functionlocal:		arg->type = local_a; break;
				case formalarg:			arg->type = formal_a; break;
				default: 				assert(0);
			}
			break; /* from case newtable_e */
		}

		/* Constants */

		case constbool_e: {
			arg->val = e->boolConst;
			arg->type = bool_a;
			break;
		}
		case conststring_e: {
			arg->val = consts_newstring(e->strConst);
			arg->type = string_a;
			break;
		}
		case constnum_e: {
			arg->val = consts_newnumber(e->numConst);
			arg->type = number_a;
			break;
		}
		case nil_e: arg->type = nil_a; break;

		/* Functions */

		case programfunc_e: {
			arg->type = userfunc_a;
			arg->val = funcIndex(e->sym->value.funcVal->name);
			break;
		}

		case libraryfunc_e: {
        	arg->type = libfunc_a;
           	arg->val = consts_newlibfunc(e->sym->value.funcVal->name);
      		break;
	    }

	    default:{
	    	printf("\nERROR E->type:%d\n",e->type);	
	    	assert(0);
	    }
	}
}

void make_numberoperand (vmarg* arg, double val) {
  	arg->val = consts_newnumber(val);
  	arg->type = number_a;
}

void make_booloperand (vmarg* arg, unsigned val) {
  	arg->val = val;
  	arg->type = bool_a;
}

void make_retvaloperand(vmarg* arg){
  	arg->type = retval_a;
}

unsigned consts_newstring(char *s){
	LIST_INSERT(stringConsts, STRINGSTATEMENT(s), stringElem, ADD_2_BACK);
	return totalStringConsts++;
}

unsigned consts_newnumber(double n){
	LIST_INSERT(numConsts, NUMBERSTATEMENT(n), numElem, ADD_2_BACK);
	return totalNumConsts++;
}

unsigned consts_newlibfunc(char *s){
	LIST_INSERT(namedLibfuncs, STRINGSTATEMENT(s), stringElem, ADD_2_BACK);
	return totalNamedLibfuncs++;
}

void generate (vmopcode op, quad *q){
	instruction *t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = op;
	if(q->arg1) 	make_operand(q->arg1, &(t->arg1));
	if(q->arg2) 	make_operand(q->arg2, &(t->arg2));
	if(q->result) 	make_operand(q->result, &(t->result));
	emit_t(t);
}

void generate_relational (vmopcode op, quad *q){
	instruction *t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = op;
	if(q->arg1) make_operand(q->arg1, &(t->arg1));
	if(q->arg2) make_operand(q->arg2, &(t->arg2));
	t->result.type = label_a;
	if(q->label < CURRPROCESSEDQUAD){
		t->result.val = Quads[q->label].taddress;
	} else {
		LIST_INSERT(ij_head, INCJMPSTATEMENT(NEXTINSTRUCTIONLABEL, q->label), incomplete_jump, ADD_2_FRONT);
	}
	emit_t(t);
}

unsigned funcIndex(char *s){
	userfunc *tmp;
	unsigned i = 0;
	for(tmp = userFuncs; tmp ; tmp = tmp->next, i++){
		if(!strcmp(s, tmp->id)){
			return i;	
		} 
	}
	return -1;
}

void generate_ADD(quad *q)				{ generate(add_v, q); }
void generate_SUB(quad *q)				{ generate(sub_v, q); }
void generate_MUL(quad *q)				{ generate(mul_v, q); }
void generate_DIV(quad *q)				{ generate(div_v, q); }
void generate_MOD(quad *q)				{ generate(mod_v, q); }
void generate_NEWTABLE(quad *q)			{ generate(newtable_v, q); }
void generate_TABLEGETELEM(quad *q)		{ generate(tablegetelem_v, q); }
void generate_TABLESETELEM(quad *q)		{ generate(tablesetelem_v, q); }
void generate_ASSIGN(quad *q)			{ generate(assign_v, q); }
void generate_JUMP(quad *q)				{ generate_relational(jump_v, q); }
void generate_IF_EQ(quad *q)			{ generate_relational(jeq_v, q); }
void generate_IF_NOTEQ(quad *q)			{ generate_relational(jne_v, q); }
void generate_IF_GREATER(quad *q)		{ generate_relational(jgt_v, q); }
void generate_IF_GREATEREQ(quad *q)		{ generate_relational(jge_v, q); }
void generate_IF_LESS(quad *q)			{ generate_relational(jlt_v, q); }
void generate_IF_LESSEQ(quad *q)		{ generate_relational(jle_v, q); }
void generate_PARAM(quad *q){
	instruction * t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = pusharg_v;
	if(q->result) make_operand(q->result,&(t->result));
	emit_t(t);
}
void generate_CALL(quad *q){
	instruction * t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = call_v;
	if(q->result) make_operand(q->result,&(t->result));
	emit_t(t);
}
void generate_GETRETVAL(quad *q){
	instruction * t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = assign_v;
	if(q->result) make_operand(q->result,&(t->result));
	make_retvaloperand(&(t->arg1));
	emit_t(t);
}
void generate_FUNCSTART(quad *q){
	SymbolTableEntry *f;
	instruction * t;
	if (funcStack == NULL) funcStack = newStack_func();

	if(q->result) f = q->result->sym;
	f->taddress = NEXTINSTRUCTIONLABEL;

	LIST_INSERT(f->returnList, LABELSTATEMENT(NEXTINSTRUCTIONLABEL), returnElem, ADD_2_BACK);

	q->taddress = NEXTINSTRUCTIONLABEL;

	LIST_INSERT(userFuncs, FUNCTIONSTATEMENT(f->taddress+1, f->totallocals, f->value.funcVal->name), userfunc, ADD_2_BACK);
	
	push_func(funcStack, f);
	totalUserFuncs++;

	t = (instruction *) malloc (sizeof(instruction));
	t->opcode = jump_v;
	RESET_OPERAND(&(t->arg1));
	RESET_OPERAND(&(t->arg2));
	t->result.type = label_a;
	emit_t(t);

	t = (instruction *) malloc (sizeof(instruction));
	t->srcLine= q->line;
	t->opcode = funcenter_v;
	if(q->result) make_operand(q->result, &(t->result));
	emit_t(t);
}
void generate_RETURN(quad *q){
	SymbolTableEntry *f;
	instruction * t = (instruction *) malloc (sizeof(instruction));
	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = assign_v;
	if(q->result) make_operand(q->result, &(t->arg1));
	make_retvaloperand(&(t->result));
	emit_t(t);

	f = top_func(funcStack);
	LIST_INSERT(f->returnList, LABELSTATEMENT(NEXTINSTRUCTIONLABEL), returnElem, ADD_2_BACK);

	t = (instruction *) malloc (sizeof(instruction));
	t->opcode = jump_v;
	RESET_OPERAND(&(t->arg1));
	RESET_OPERAND(&(t->arg2));
	t->result.type = label_a;
	emit_t(t);
}
void generate_FUNCEND(quad *q){
	SymbolTableEntry *f;
	instruction * t = (instruction *) malloc (sizeof(instruction));

	f = pop_func(funcStack);
	backpatchReturn(f->returnList, NEXTINSTRUCTIONLABEL);

	q->taddress = NEXTINSTRUCTIONLABEL;
	t->srcLine= q->line;
	t->opcode = funcexit_v;
	if(q->result) make_operand(q->result, &(t->result));
	emit_t(t);
}
void generate_NOP(quad *quad){ 
	instruction *t; 
	t = malloc(sizeof(instruction)); 
	t->opcode = nop_v; 
	emit_t(t); 
}

void patch_incomplete_jumps(){
    incomplete_jump *tmp = ij_head;
    for(tmp = ij_head; tmp ; tmp=tmp->next){
        if (tmp->iaddress==currProcessed){
            Instructions[tmp->instrNo].result.val= NEXTINSTRUCTIONLABEL;
        } else {
            Instructions[tmp->instrNo].result.val = Quads[tmp->iaddress].taddress;
        }
    }
}

void expand_t(){
	assert(total_t == currInstruction);
	instruction *t = (instruction*) malloc (NEW_SIZE_T);
	if (Instructions) {
		memcpy(t, Instructions, CURR_SIZE_T);
		free(Instructions);
	}
	Instructions = t;
	total_t += EXPAND_SIZE_T;
}

void emit_t(instruction * t){
	if(currInstruction == total_t)
		expand_t();

	instruction * i = malloc(sizeof(instruction));
	i= Instructions+currInstruction++;
	i->opcode = t->opcode;
	i->arg1 = t->arg1;
	i->arg2 = t->arg2;
	i->result = t->result;
	i->srcLine = t->srcLine;
}

returnElem * backpatchReturn(returnElem *head, unsigned label){
	if(head){
		returnElem * tmp;
		Instructions[head->label].result.val = label+1;
		for(tmp = head->next; tmp; tmp = tmp->next){
			Instructions[tmp->label].result.val = label;
		}
	}
	return;
}

void print_instruction(instruction * t){
      assert(t);
      char * vmarguments[] = {
	    "label",
	    "global",
	    "formal",
	    "local" ,
	    "number",
	    "string",
	    "bool",
	    "nil",
	    "userfunc",
	    "libfunc",
	    "retval"
	};

	switch (t->opcode){
	case add_v: printf("add\t\t"); goto twoArgs;
	case sub_v: printf("sub\t\t"); goto twoArgs;
	case mul_v: printf("mul\t\t"); goto twoArgs;
	case div_v: printf("div\t\t"); goto twoArgs;
	case mod_v: printf("mod\t\t"); goto twoArgs;
	case jeq_v: printf("jeq\t\t"); goto twoArgs;
	case jne_v: printf("jne\t\t"); goto twoArgs;
	case jle_v: printf("jle\t\t"); goto twoArgs;
	case jge_v: printf("jge\t\t"); goto twoArgs;
	case jlt_v: printf("jlt\t\t"); goto twoArgs;
	case jgt_v: printf("jgt\t\t"); goto twoArgs;
	case tablegetelem_v: printf("tablegetelem\t"); goto twoArgs;
	case tablesetelem_v: printf("tablesetelem\t");
		twoArgs:
		printf("(%s), %3d\t\t (%s), %3d \t\t (%s), %3d   \n",vmarguments[t->result.type],t->result.val,vmarguments[t->arg1.type],t->arg1.val,vmarguments[t->arg2.type],t->arg2.val);
		break;
	case assign_v:
		printf("assign\t\t");
		printf("(%s), %3d\t\t (%s), %3d \n",vmarguments[t->result.type],t->result.val, vmarguments[t->arg1.type], t->arg1.val);
		break;
	case call_v: printf("call\t\t"); goto nextArgs; 
	case jump_v: printf("jump\t\t"); goto nextArgs;
	case pusharg_v: printf("pusharg\t\t"); goto nextArgs;
	case funcenter_v: printf("funcenter\t\t"); goto nextArgs;
	case funcexit_v: printf("funcexit\t\t"); goto nextArgs;
	case newtable_v: printf("newtable\t\t");
		nextArgs:
		printf("(%s), %3d \n",vmarguments[t->result.type],t->result.val);
		break;
	default:
		printf("invalid opcode\n");
		assert(0);
	} 
}

void printInstructions(){
    int i;
    numElem *tmpNum;
    stringElem *tmpStr;
    userfunc *tmpFunc;

    printf("\n- [NUM ARRAY] -\n");
    for(tmpNum = numConsts, i=0; tmpNum ; tmpNum = tmpNum->next, i++){
    	printf("[%3d] %f\n", i, tmpNum->num);
    }
    printf("\n- [STRING ARRAY] -\n");
    for(tmpStr = stringConsts, i=0; tmpStr ; tmpStr = tmpStr->next, i++){
    	printf("[%3d] %s\n", i, tmpStr->s);
    }
    printf("\n- [USER FUNC ARRAY] -\n");
    for(tmpFunc = userFuncs, i=0; tmpFunc ; tmpFunc = tmpFunc->next, i++){
    	printf("[%3d] %s, address: %d, locals: %d\n", i, tmpFunc->id,tmpFunc->address, tmpFunc->localSize);
    }
    printf("\n- [LIB FUNC ARRAY] -\n");
    for(tmpStr = namedLibfuncs, i=0; tmpStr ; tmpStr = tmpStr->next, i++){
    	printf("[%3d] %s\n", i, tmpStr->s);
    }

   	printf("\n- [INSTRUCTIONS] -\n");
   	printf("[index // opcode // \tresult(type),offset // \targ1(type),offset // \typearg2(type),offset]\n");
	for(i = 0; i < currInstruction;i++){
       	printf("[%3d] ",i);
		print_instruction(Instructions+i);
   	}
}
