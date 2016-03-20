#include "symtable.h"

ScopeList * scopelist = NULL;
SymTable_T * symTable = NULL;
int curr_scope = 0;
SymbolTableEntry * last_node = NULL;
SymbolTableType last_type = 0;
ScopeNode * scope_node = 0;
char * tmp;
int func_line = 0;

ScopeList * insertScopeList(ScopeList *head, int scope){
 	ScopeList *newE1;

 	newE1 = (ScopeList *) malloc (sizeof(ScopeList));
 	
 	newE1->scope = scope;
 	newE1->head = NULL;
	newE1->next = head;
	head = newE1;

	return newE1;
}

void hide(ScopeList *head, int scope){
	ScopeList *tmp;
	ScopeNode *node;
	for(tmp=head;tmp;tmp=tmp->next){
		if(tmp->scope == scope){
			for(node = tmp->head;node;node=node->next){
				node->symbol->isActive = 0;
			}
		}
	}
	return;
}

ScopeList * lookupScopeList(ScopeList *head, int scope){
 	ScopeList *tmp;
 	for(tmp=head; tmp; tmp=tmp->next){
 		if(tmp->scope == scope){
 			return tmp;
 		}
 	}
 	return NULL;
 }

ScopeNode * lookup(ScopeList *head, int scope, char *name){
	ScopeList *tmp;
	ScopeNode *it, *res = NULL;

	for(tmp=head; tmp; tmp=tmp->next){
		if(tmp->scope <= scope){
			for(it = tmp->head;it;it=it->next){
				if(strcmp(getSymName(it->symbol),name)==0 && it->symbol->isActive){
					if(res && getSymScope(res->symbol) < getSymScope(it->symbol))
						res = it;
					if(!res)
						res = it;
				}
			}
		}
	}
	return res;
}

ScopeList * insertScopeNode(ScopeList *head, SymbolTableEntry *sym){
 	ScopeList *tmp;
 	ScopeNode *newE1;

 	newE1 = (ScopeNode *) malloc (sizeof(ScopeNode));

	tmp = lookupScopeList(head, getSymScope(sym));
	if(tmp==NULL){
		tmp = insertScopeList(head, getSymScope(sym));
		head = tmp;
	}

 	newE1->symbol = sym;
 	newE1->next = tmp->head;
 	tmp->head = newE1;

 	return head;
}

static unsigned int hash(const char *pcKey, const int buckets){
	register size_t ui;
	unsigned int uiHash = 0U;
	for (ui = 0U; pcKey[ui] != '\0'; ui++)
		uiHash = uiHash * HASH_MULTIPLIER + pcKey[ui];
	return uiHash%buckets;
}

SymTable_T * SymTable_new(void){
	SymTable_T *new;
	new = (SymTable_T *) malloc (sizeof(SymTable_T)) ;
	if (new == NULL) {
		printf("ERROR: could not allocate new symtable\n");
		exit(EXIT_FAILURE);
	}
	new->size = 0;
	new->length = DEFAULT_SIZE;
	new->hashtable = (SymbolTableEntry**) calloc (DEFAULT_SIZE,sizeof(SymbolTableEntry*));
	return new;
}

SymbolTableEntry * put(SymTable_T *oSymTable, ScopeList **oScopeList, char *name, unsigned int scope, unsigned int line, SymbolTableType type, symbol_t symbolType, scopespace_t scopeSpace){
	SymbolTableEntry *newE1, *tmp;
	unsigned int key;

	if( (tmp = SymTable_lookup(oSymTable,name,scope)) == NULL){
		newE1 = (SymbolTableEntry *) malloc (sizeof(SymbolTableEntry));
		newE1->isActive = 1;
		newE1->type = type;
		newE1->symbolType = symbolType;
		newE1->scopeSpace = scopeSpace;

		if(type == GLOBAL_T || type == LOCAL_T || type == FORMAL_T){
			newE1->value.varVal = (Variable *) malloc (sizeof(Variable));
			newE1->value.varVal->name = strdup(name);
			newE1->value.varVal->scope = scope;
			newE1->value.varVal->line = line;
		}else{
			newE1->value.funcVal = (Function *) malloc (sizeof(Function));
			newE1->value.funcVal->name = strdup(name);
			newE1->value.funcVal->scope = scope;
			newE1->value.funcVal->args = NULL;
			newE1->value.funcVal->line = line;
		}

		key = hash(name,oSymTable->length);
		newE1->next = oSymTable->hashtable[key];
		oSymTable->hashtable[key] = newE1;

		*oScopeList = insertScopeNode(*oScopeList,newE1);

		oSymTable->size++;

		return newE1;
	}

	return tmp;
}

SymbolTableEntry * insertArgsFunction(SymTable_T *oSymTable, ScopeList **oScopeList, SymbolTableEntry *func, char *name, unsigned int scope, unsigned int line){
	SymbolTableEntry * tmp;
	ListArgs *ltmp;
	tmp = put(oSymTable, oScopeList, name, scope, line, FORMAL_T, var_s, formalarg);

	if(tmp){
		ltmp = (ListArgs *) malloc (sizeof(ListArgs));
		ltmp->var = tmp->value.varVal;

		if(func->value.funcVal->args == NULL){
			ltmp->next = NULL;
			func->value.funcVal->args = ltmp;
			return tmp;
		}

		ltmp->next = func->value.funcVal->args;
		func->value.funcVal->args = ltmp;
 	}

	return tmp;	
}

SymbolTableEntry * SymTable_lookup(SymTable_T *oSymTable, char *name, unsigned int scope){
	unsigned int key;
	SymbolTableEntry *tmp;
	key = hash(name,oSymTable->length);
	for(tmp = oSymTable->hashtable[key];tmp;tmp=tmp->next){
		if((strcmp(getSymName(tmp),name)==0) && 
			(getSymScope(tmp) == scope) &&
			(tmp->isActive == 1)
			) return tmp;
	}
	return NULL;
}

void printHashTable(SymTable_T *head){
	SymbolTableEntry **tmp = head->hashtable;
	SymbolTableEntry *list;
	ListArgs *tmpargs;
	int i;
	printf("\n- [HASHTABLE] -\n");
	for(i=0;i<head->length;i++){
		if(tmp[i]!=NULL){
			for(list=tmp[i];list;list=list->next){
				if(list->type == 0 || list->type == 1 || list->type == 2){
					printf("isActive: %s, type: %s, name: %s, line: %d, scope: %d, offset: %d\n",list->isActive ? "TRUE" : "FALSE",printType(list->type),getSymName(list),getSymLine(list),getSymScope(list),list->offset);
				}else{
					printf("isActive: %s, type: %s, name: %s, line: %d, scope: %d, offset: %d\n",list->isActive ? "TRUE" : "FALSE",printType(list->type),getSymName(list),getSymLine(list),getSymScope(list),list->offset);
					for(tmpargs = list->value.funcVal->args; tmpargs; tmpargs=tmpargs->next){
						printf("\tname: %s, line: %d, scope: %d\n",tmpargs->var->name,tmpargs->var->line,tmpargs->var->scope,tmpargs->var->line);
					}
				}
			}
		}
	}
}

void printScopeList(ScopeList *head){
	ScopeList *tmp;
	ScopeNode *tmp_2;
	printf("\n- [SCOPELIST] -\n");
	for(tmp=head;tmp;tmp=tmp->next){
		for(tmp_2=tmp->head;tmp_2;tmp_2=tmp_2->next){
			printf("name: %s, scope: %d, line %d\n",getSymName(tmp_2->symbol),getSymScope(tmp_2->symbol),getSymLine(tmp_2->symbol));
		}
	}
}

char * printType(SymbolTableType type){
	char * tmp;
	switch(type){
		case GLOBAL_T: 	return (tmp = strdup("GLOBAL_T"));
		case LOCAL_T: 	return (tmp = strdup("LOCAL_T"));
		case FORMAL_T:	return (tmp = strdup("FORMAL_T"));
		case USERFUNC_T:return (tmp = strdup("USERFUNC_T"));
		case LIBFUNC_T:	return (tmp = strdup("LIBFUNC_T"));
		default:
			assert(0);
	}
}

char * getSymName(SymbolTableEntry * sym){
	if(sym->type == GLOBAL_T || sym->type == LOCAL_T || sym->type == FORMAL_T ){
		return sym->value.varVal->name;
	} else {
		return sym->value.funcVal->name;
	}
	return NULL;
}

int getSymLine(SymbolTableEntry * sym){
	if(sym->type == GLOBAL_T || sym->type == LOCAL_T || sym->type == FORMAL_T ){
		return sym->value.varVal->line;
	} else {
		return sym->value.funcVal->line;
	}
	return -1;
}

int getSymScope(SymbolTableEntry * sym){
	if(sym->type == GLOBAL_T || sym->type == LOCAL_T || sym->type == FORMAL_T ){
		return sym->value.varVal->scope;
	} else {
		return sym->value.funcVal->scope;
	}
	return -1;
}
