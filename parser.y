%{
	#include <stdio.h>
	#include <stdlib.h>
  	#include <string.h>

  	#include "symtable.h"
  	#include "intermediateCode.h"
  	#include "targetCode.h"
  	#include "stack.h"
  	#include "list.h"

	int yyerror(char* yaccProvidedMessage);
	int yylex(void);

	extern int yylineno;
	extern char* yytext;
	extern FILE* yyin;
	extern FILE* yyout;

	int func_lvalue = 0;
	int func_args = 0;
	int debug_print = 1;
	int loopcounter = 0;
	Stack *loopcounterstack = NULL;
	Stack *scopeoffsetstack = NULL;
	Stack *functioncounterstack = NULL;
	stmtStruct *statementscounterlist = NULL;
%}

%union {
	char* 						stringValue;
	int 						intValue;
	double						realValue;
	struct expr					*exprValue;
	struct func 				*funcValue;
	struct SymbolTableEntry 	*symValue; 
	struct forStruct			*forValue;
	struct stmtStruct			*stmtValue;
	struct booleanlist			*boolValue;
}

%start program

%token <stringValue>	ID STRING GREATEREQUAL LESSEQUAL INCRE DECRE TWOPERIOD EQUAL NOTEQUAL ACCESSOR
%token <intValue>		INTEGER
%token <realValue>		REAL
%token <stringValue>	FUNCTION TRUE FALSE NIL IF ELSE FOR WHILE RETURN CONTINUE BREAK AND OR NOT LOCAL
%token <stringValue>	ASSIGN GREATER LESS PLUS MINUS MOD DIV MULT LBRACE RBRACE LBRACKET RBRACKET LPARENTH RPARENTH
%token <stringValue>	SEMICOLON COMMA COLON PERIOD

%type <exprValue>		lvalue assignexpr expr const term primary call member logic
%type <exprValue>		elist objectdef indexed indexedelem indexedelems reversedelist reversedelists

%type <symValue>		funcdef funcprefix
%type <intValue>		funcbody elseprefix whilestart whilecond N M K
%type <funcValue>		normcall callsuffix methodcall
%type <stringValue>		funcname
%type <forValue>		forprefix
%type <boolValue>		ifprefix

%right		ASSIGN
%left		OR
%left		AND
%nonassoc	EQUAL NOTEQUAL
%nonassoc	GREATER GREATEREQUAL LESS LESSEQUAL
%left		PLUS MINUS
%left		MULT DIV MOD
%right		NOT INCRE DECRE UMINUS
%left		PERIOD TWOPERIOD
%left		LBRACKET RBRACKET 
%left		LPARENTH RPARENTH

%nonassoc	IFX
%nonassoc	ELSE

%%

program:		statements	{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'program'.\n"); 
							}
				;

statements:		stmt statements { 
									if(debug_print) fprintf(yyout, "Reduced rule 'stmt statements'.\n");
								}
				|
				;

stmt:			expr SEMICOLON 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr ;'\n");
												backpatch($1->truelist, NEXTQUAD);
												backpatch($1->truelist, NEXTQUAD);
											}
				| ifstmt					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'ifstmt'\n"); 
											}
				| whilestmt					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'whilestmt'\n"); 
											}
				| forstmt					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'forstmt'\n");

											}
				| returnstmt				{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'returnstmt'\n");
											}
				| BREAK SEMICOLON			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'break ;'\n"); 
												if(isEmpty(loopcounterstack)){
													tmp = strdup("error: cannot break outside of loop");
													yyerror(tmp);
													return -1;
												}
												if(!statementscounterlist) LIST_INSERT(statementscounterlist, STMTSTATEMENT, stmtStruct, ADD_2_FRONT);
												LIST_INSERT(statementscounterlist->breaklist, BRCNTSTATEMENT(currQuad), breakElem, ADD_2_FRONT);
												emit_i(jump, NULL, NULL, NULL, -1, yylineno);
											}
				| CONTINUE SEMICOLON		{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'continue ;\n"); 
												if(isEmpty(loopcounterstack)){
													tmp = strdup("error: cannot continue outside of loop");
													yyerror(tmp);
													return -1;
												}
												if(!statementscounterlist) LIST_INSERT(statementscounterlist, STMTSTATEMENT, stmtStruct, ADD_2_FRONT);
												LIST_INSERT(statementscounterlist->contlist, BRCNTSTATEMENT(currQuad), contElem, ADD_2_FRONT);
												emit_i(jump, NULL, NULL, NULL, -1, yylineno);
											}
				| block						{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'block'\n"); 
											}
				| funcdef					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'funcdef'\n");
											}
				| SEMICOLON					{ 
												if(debug_print) fprintf(yyout, "Reduced rule ';'\n");
											}
				;

expr:			assignexpr					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'assignexpr'\n"); 
												$$ = $1;
											}
				| expr PLUS expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr + expr'\n");
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, add, yylineno);
											}
				| expr MINUS expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr - expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, sub, yylineno);
											}
				| expr MULT expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr * expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, mul, yylineno);
											}
				| expr DIV expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr / expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, divi, yylineno);
											}
				| expr MOD expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr %% expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, mod, yylineno);
											}
				| expr GREATER expr 		{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr > expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_greater, yylineno);
											}
				| expr GREATEREQUAL expr 	{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr >= expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_greatereq, yylineno);
											}
				| expr LESS expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr < expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_less, yylineno);
											}
				| expr LESSEQUAL expr 		{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr <= expr'\n"); 
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_lesseq, yylineno);
											}
				| expr EQUAL expr 			{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr == expr'\n");
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_eq, yylineno);
											}
				| expr NOTEQUAL expr 		{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr != expr'\n");
												checkoperands($1);
												checkoperands($3);
												$$ = emit_operation($1, $3, if_noteq, yylineno);
											}
				| term 						{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'term'\n"); 
												$$ = $1;
											}
				| logic						{
												$$ = $1;
											}
				;

logic:			expr 						{ if($1->type != boolexpr_e) $1 = emit_operation($1, newexpr_constbool(1), if_eq, yylineno); }
				AND K expr					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr && expr'\n"); 
												$$ = newexpr(boolexpr_e);
												if($5->type != boolexpr_e){
													$5 = emit_operation($5, newexpr_constbool(1), if_eq, yylineno);
												}
												backpatch($1->truelist, $4);
												$$->truelist = $5->truelist;
												$$->falselist = merge($1->falselist, $5->falselist);
											}
				| expr 						{ if($1->type != boolexpr_e) $1 = emit_operation($1, newexpr_constbool(1), if_eq, yylineno); }
				OR K expr					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'expr || expr'\n"); 
												$$ = newexpr(boolexpr_e);
												if($5->type != boolexpr_e){
													$5 = emit_operation($5, newexpr_constbool(1), if_eq, yylineno);
												}
												backpatch($1->falselist, $4);
												$$->truelist = merge($1->truelist, $5->truelist);
												$$->falselist =  $5->falselist;
											}
				;

term:			LPARENTH expr RPARENTH		{ 
												if(debug_print) fprintf(yyout, "Reduced rule '( expr )'\n"); 
												$$ = $2;
											}
				| MINUS expr %prec UMINUS	{ 
												if(debug_print) fprintf(yyout, "Reduced rule '- expr'\n"); 
												checkuminus($2);
												$$ = newexpr(arithexpr_e);
												$$->sym = istempexpr($2)? $2->sym : newtemp();
												emit_i(mul, $2, newexpr_constnum(-1), $$, currQuad, yylineno);
											}
				| NOT expr 					{ 
												if(debug_print) fprintf(yyout, "Reduced rule '! expr'\n");
												$$ = newexpr(boolexpr_e);
												$$->sym = $2->sym;
												$$->truelist = $2->falselist;
												$$->falselist = $2->truelist;	
											}
				| INCRE lvalue 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule '++lvalue'\n"); 
												if($2->type == tableitem_e){
													$$ = emit_iftableitem($2);
													emit_i(add, $$, newexpr_constnum(1), $$, currQuad, yylineno);
													emit_i(tablesetelem, $$, $2->index, $2, currQuad, yylineno);
												}
												else{
													emit_i(add, $2, newexpr_constnum(1), $2, currQuad, yylineno);
													$$ = newexpr(arithexpr_e);
													$$->sym = newtemp();
													emit_i(assign, $2, (expr *)0, $$, currQuad, yylineno);
												}
											}
				| lvalue INCRE 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'lvalue++'\n"); 
												$$=newexpr(var_e);
												$$->sym = newtemp();
												if($1->type == tableitem_e){
													expr *t = emit_iftableitem($1);
													emit_i(assign, t, (expr *)0, $$, currQuad, yylineno);
													emit_i(add, t, newexpr_constnum(1), t, currQuad, yylineno);
													emit_i(tablesetelem, $$, $1->index, $1, currQuad, yylineno);
												}
												else{
													emit_i(assign, $1, (expr *)0, $$, currQuad, yylineno);
													emit_i(add, $1, newexpr_constnum(1), $1, currQuad, yylineno);
												}
											}
				| DECRE lvalue 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule '--lvalue'\n"); 
												if($2->type == tableitem_e){
													$$ = emit_iftableitem($2);
													emit_i(sub, $$, newexpr_constnum(1), $$, currQuad, yylineno);
													emit_i(tablesetelem, $$, $2->index, $2, currQuad, yylineno);
												}
												else{
													emit_i(sub, $2, newexpr_constnum(1), $2, currQuad, yylineno);
													$$ = newexpr(arithexpr_e);
													$$->sym = newtemp();
													emit_i(assign, $2, (expr *)0, $$, currQuad, yylineno);
												}
											}
				| lvalue DECRE 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'lvalue--'\n"); 
												$$=newexpr(var_e);
												$$->sym = newtemp();
												if($1->type == tableitem_e){
													expr *t = emit_iftableitem($1);
													emit_i(assign, t, (expr *)0, $$, currQuad, yylineno);
													emit_i(sub, t, newexpr_constnum(1), t, currQuad, yylineno);
													emit_i(tablesetelem, $1, $1->index, $$, currQuad, yylineno);
												}
												else{
													emit_i(assign, $1, (expr *)0, $$, currQuad, yylineno);
													emit_i(sub, $1, newexpr_constnum(1), $1, currQuad, yylineno);
												}
											}
				| primary 					{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'primary'\n"); 
												$$ = $1;
											}
				;

assignexpr: 	lvalue 						{ 	
												if(func_lvalue){
													yyerror("error: cannot use function as lvalue");
													return -1;
												}
											} 
				ASSIGN expr 				{ 
												if(debug_print) fprintf(yyout, "Reduced rule 'lvalue = expr'\n"); 
												if($1->type == tableitem_e){
													emit_i(tablesetelem, $4, $1->index, $1, currQuad, yylineno);
													$$ = emit_iftableitem($1);
													$$->type = assignexpr_e;
												}else{
													if($4->type == boolexpr_e){
														$$ = newexpr(assignexpr_e);
														$$->sym = newtemp();
														backpatch($4->truelist, NEXTQUAD);
														emit_i(assign, newexpr_constbool(1), 0, $$, currQuad, yylineno);
														emit_i(jump, NULL, NULL, NULL, currQuad+2, yylineno);
														backpatch($4->falselist, NEXTQUAD);
														emit_i(assign, newexpr_constbool(0), NULL, $$, currQuad, yylineno);
														emit_i(assign, $$, NULL, $1, currQuad, yylineno);
														$$ = newexpr(assignexpr_e);
														$$->sym = newtemp();
														emit_i(assign, $1, NULL, $$, currQuad, yylineno);
													} else {
														emit_i(assign, $4, NULL, $1, currQuad, yylineno);
														$$ = newexpr(assignexpr_e);
														$$->sym = newtemp();
														emit_i(assign, $1, NULL, $$, currQuad, yylineno);
													}
												}
											}
				;

primary:		lvalue 							{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'lvalue'\n"); 
													$$ = emit_iftableitem($1);
												}
				| call 							{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'call'\n"); 
												}
				| objectdef 					{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'objectdef'\n"); 
													$$ = $1;
												}
				| LPARENTH funcdef RPARENTH 	{ 
													if(debug_print) fprintf(yyout, "Reduced rule '( funcdef )'\n"); 
													$$ = newexpr(programfunc_e);
													$$->sym = $2;
												}
				| const 						{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'const'\n"); 
													$$ = $1;
												}
				;

lvalue:			ID 						{ 
											if(debug_print) fprintf(yyout, "Reduced rule 'ID'\n"); 
											func_lvalue = 0;
											if( ((scope_node = lookup(scopelist, curr_scope, yytext)) == NULL) ||
												(scope_node->symbol->isActive == 0)
												){
												(curr_scope == 0) ? (last_type = GLOBAL_T) : (last_type = LOCAL_T);
												last_node = put(symTable, &scopelist, yytext, curr_scope, yylineno, last_type, var_s, programvar);
												last_node->scopeSpace = currscopespace();
												last_node->offset = currscopeoffset();
												inccurrscopeoffset();
											}
											else if( 
												(getSymLine(scope_node->symbol) < func_line ) &&
												(getSymScope(scope_node->symbol) < curr_scope && getSymScope(scope_node->symbol) != 0)
											)
											{
												return yyerror("error: cannot access");
											}
											else if(scope_node->symbol->type == USERFUNC_T || scope_node->symbol->type == LIBFUNC_T){
												func_lvalue = 1;
											}
											$$ = lvalue_expr(lookup(scopelist, curr_scope, yytext)->symbol);
										}
				| LOCAL ID 				{ 	
											func_lvalue = 0;
											if(debug_print) fprintf(yyout, "Reduced rule 'LOCAL ID'\n"); 
											if( ((scope_node = lookup(scopelist, curr_scope, yytext)) == NULL) ||
												 (scope_node->symbol->isActive == 0)
												){
												(curr_scope == 0) ? (last_type = GLOBAL_T) : (last_type = LOCAL_T);
												last_node = put(symTable, &scopelist, yytext, curr_scope, yylineno, last_type, var_s, programvar);
												last_node->scopeSpace = currscopespace();
												last_node->offset = currscopeoffset();
												inccurrscopeoffset();
											}
											else if(scope_node->symbol->type == LIBFUNC_T){
												tmp = strdup("error: collision with library function: ");
												strcat(tmp,yytext);
												yyerror(tmp);
												return -1;
											}
											else if((scope_node->symbol->type != LOCAL_T) && (getSymScope(scope_node->symbol) == curr_scope)){
												tmp = strdup("error: redeclaration of ");
												strcat(tmp,yytext);
												strcat(tmp," as local");
												yyerror(tmp);
												return -1;
											}
											else if(getSymScope(scope_node->symbol) < curr_scope){
												(curr_scope == 0) ? (last_type = GLOBAL_T) : (last_type = LOCAL_T);
												last_node = put(symTable, &scopelist, yytext, curr_scope, yylineno, last_type, var_s, programvar);
												last_node->scopeSpace = currscopespace();
												last_node->offset = currscopeoffset();
												inccurrscopeoffset();
											}
											$$ = lvalue_expr(lookup(scopelist, curr_scope, yytext)->symbol);
										}
				| ACCESSOR ID 			{ 
											func_lvalue = 0;
											if(debug_print) fprintf(yyout, "Reduced rule '::'\n"); 
											if( (scope_node = lookup(scopelist, 0, yytext)) == NULL){
												tmp = strdup("error: no global variable ");
												strcat(tmp,yytext);
												yyerror(tmp);
												return -1;
											}
											$$ = lvalue_expr(scope_node->symbol);
										}
				| member				{ 
											func_lvalue = 0;
											if(debug_print) fprintf(yyout, "Reduced rule 'member'\n"); 
											$$ = $1;
										}
				;

member:			lvalue PERIOD ID 					{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'lvalue . ID'\n"); 
														$$ = member_item($1,$3);
													}
				| lvalue LBRACKET expr RBRACKET 	{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'lvalue [ expr ]'\n"); 
														$1 = emit_iftableitem($1);
														$$ = newexpr(tableitem_e);
														$$->sym = $1->sym;
														$$->index = $3;
													}
				| call PERIOD ID 					{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'call . ID'\n"); 
														$$ = member_item($1,$3);
													}
				| call LBRACKET expr RBRACKET		{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'call [ epxr ]'\n"); 
														$$ = member_item($1,getSymName($3->sym));
													}
				;

call:			call LPARENTH elist RPARENTH							{ 
																			if(debug_print) fprintf(yyout, "Reduced rule 'call ( elist )'\n"); 
																			$$ = make_call($1,$3,yylineno);
																		}
				| lvalue callsuffix										{ 	
																			if(debug_print) fprintf(yyout, "Reduced rule 'lvalue callsufix'\n"); 
																			if($2->method){
																				expr *self = $1;
																				expr *tmp;
																				$1 = emit_iftableitem(member_item(self, $2->name));
																				for(tmp = $2->elist; tmp && tmp->next; tmp = tmp->next){
																					printf("name: %s\n",tmp->sym->value.varVal->name);
																				}
																				if(tmp) tmp->next = self;
																				else tmp = self;
																			}
																			$$ = make_call($1, $2->elist,yylineno);
																		}
				| LPARENTH funcdef RPARENTH LPARENTH elist RPARENTH 	{ 
																			if(debug_print) fprintf(yyout, "Reduced rule '( funcdef ) ( elist )'\n"); 
																			expr *newfunc = newexpr(programfunc_e);
																			newfunc->sym = $2;
																			$$ = make_call(newfunc,$5,yylineno);
																		}
				;
	
callsuffix:		normcall		{ 
									if(debug_print) fprintf(yyout, "Reduced rule 'normcall'\n"); 
									$$ = $1;
								}
				| methodcall	{ 
									if(debug_print) fprintf(yyout, "Reduced rule 'methodcall'\n");
									$$ = $1;
								}
				;

normcall:		LPARENTH elist RPARENTH 	{ 
												if(debug_print) fprintf(yyout, "Reduced rule '( elist )'\n"); 
												$$ = malloc (sizeof(func));
												$$->elist = $2;
												$$->method = 0;
												$$->name = NULL;
											}
				;

methodcall:		TWOPERIOD ID 				{
												if( ((scope_node = lookup(scopelist, curr_scope, yytext)) == NULL) ||
													(scope_node->symbol->isActive == 0)
													){
													(curr_scope == 0) ? (last_type = GLOBAL_T) : (last_type = LOCAL_T);
													last_node = put(symTable, &scopelist, yytext, curr_scope, yylineno, last_type, var_s, programvar);
													last_node->scopeSpace = currscopespace();
													last_node->offset = currscopeoffset();
												inccurrscopeoffset();
												}
												else if(
													(getSymLine(scope_node->symbol) < func_line ) &&
													(getSymScope(scope_node->symbol) < curr_scope && getSymScope(scope_node->symbol) != 0)
												)
												{
													yyerror("error: cannot access");
													return -1;
												}
											}

				LPARENTH elist RPARENTH 	{ 
												if(debug_print) fprintf(yyout, "Reduced rule '.. ID ( elist )'\n"); 
												$$ = malloc (sizeof(func));
												$$->elist = $5;
												$$->method = 1;
												$$->name = strdup($2);
											}
				;

elist:			elist COMMA expr 	{
										if(debug_print) fprintf(yyout, "Reduced rule 'elist , expr'\n"); 
										$$ = $3;
										$$->next = $1;
									}
				| expr 				{
										if(debug_print) fprintf(yyout, "Reduced rule 'expr'\n");
										$$ = $1;		
									}
				|					{
										$$ = NULL;
									}
				;

reversedelist:	expr reversedelists {
										if(debug_print) fprintf(yyout, "Reduced rule 'expr reversedelists'\n");
										$$ = $1;
										$$->next = $2;
									}
				|					{
										$$ = NULL;
									}
				;

reversedelists: COMMA expr reversedelists 	{
												if(debug_print) fprintf(yyout, "Reduced rule ', expr reversedelists'\n");
												$$ = $2;
												$$->next = $3;
											}
				|							{
												$$ = NULL;
											}
				;

objectdef:		LBRACKET reversedelist RBRACKET 	{ 
												if(debug_print) fprintf(yyout, "Reduced rule '[ reversedelist ]'\n"); 
												expr *t = newexpr(newtable_e);
												t->sym = newtemp();
												emit_i(tablecreate, (expr *)0, (expr *)0, t, currQuad, yylineno);
												int i=0;
												for(;$2;$2 = $2->next){
													emit_i(tablesetelem, $2, newexpr_constnum(i++), t, currQuad, yylineno);
												}
												$$ = t;
											}
				| LBRACKET indexed RBRACKET	{ 
												if(debug_print) fprintf(yyout, "Reduced rule '[ indexed ]'\n"); 
												expr *t = newexpr(newtable_e);
												t->sym = newtemp();
												emit_i(tablecreate, (expr *)0, (expr *)0, t, currQuad, yylineno);
												for(;$2;$2 = $2->next){
													emit_i(tablesetelem, $2->index, $2, t, currQuad, yylineno);
												}
												$$ = t;
											}
				;

indexed:		indexedelem indexedelems 		{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'indexedelem indexedelems'\n"); 
													$$ = $1;
													$$->next = $2;
												}
				;

indexedelem:	LBRACE expr COLON expr RBRACE 	{ 
													if(debug_print) fprintf(yyout, "Reduced rule '{ epxr : expr }'\n"); 
													$$ = $2;
													$$->index = $4;
												}
				;

indexedelems:	COMMA indexedelem indexedelems 	{ 
													if(debug_print) fprintf(yyout, "Reduced rule ', indexedelem indexedelems'.\n"); 
													$$ = $2;
													$$->next = $3;
												}
				|								{
													$$ = NULL;
												}
				;

block:			LBRACE 							{ 
													curr_scope++; 
												} 
				statements RBRACE				{ 
													if(debug_print) fprintf(yyout, "Reduced rule '{ statemets }'\n");
													hide(scopelist,curr_scope); 
													curr_scope--;
												}
				;

funcname:		ID 	{ 
						$$ = strdup(yytext);
					}
				|	{
						$$ = strdup(newtempfuncname());
					}
				;

funcprefix:		FUNCTION funcname	{
										func_line = yylineno;
										last_type = USERFUNC_T;
										if( ((scope_node = lookup(scopelist, curr_scope, $2)) == NULL) ||
											(scope_node->symbol->isActive == 0)
											){
											last_node = put(symTable, &scopelist, $2, curr_scope, yylineno, USERFUNC_T, programfunc_s, functionlocal);
											last_node->value.funcVal->iaddress = currQuad;
											last_node->scopeSpace = currscopespace();
											last_node->offset = currscopeoffset();
											$$ = last_node;
										}
										else if(scope_node->symbol->type == LIBFUNC_T){
											tmp = strdup("error: collision with library function: ");
											strcat(tmp,$2);
											yyerror(tmp);
											return -1;
										}
										else if((scope_node->symbol->type != USERFUNC_T) && (getSymScope(scope_node->symbol) == curr_scope)){
											tmp = strdup("error: redeclaration of variable:");
											strcat(tmp,$2);
											strcat(tmp," as function");
											yyerror(tmp);
											return -1;
										}
										else if(getSymScope(scope_node->symbol) == curr_scope){
											tmp = strdup("error: redeclaration of function: ");
											strcat(tmp,$2);
											yyerror(tmp);
											return -1;
										}
										else if(getSymScope(scope_node->symbol) < curr_scope){
											last_node = put(symTable, &scopelist, $2, curr_scope, yylineno, USERFUNC_T, programfunc_s, functionlocal);
											last_node->value.funcVal->iaddress = currQuad;
											last_node->scopeSpace = currscopespace();
											last_node->offset = currscopeoffset();
											$$ = last_node;
										}
										emit_i(funcstart, NULL, NULL, lvalue_expr($$), currQuad, yylineno);
										push(&scopeoffsetstack, currscopeoffset());
										ENTERSCOPESPACE;
										RESETFORMALARGSOFFSET;
									}
				;

funcargs:		LPARENTH	{
								func_args = 1;
								curr_scope++;
							}
				idlist		
				RPARENTH	{
								func_args = 0;
								curr_scope--;

								ENTERSCOPESPACE;
								RESETFUNCTIONLOCALOFFSET;
							}
				;

funcblockstart:	{
					push(&functioncounterstack, curr_scope);
					loopcounter = 0;
				}
				;

funcblockend:	{
					pop(&functioncounterstack);
				}
				;

funcbody:		funcblockstart block funcblockend 	{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'function ID ( idlist ) block'\n");
														$$ = currscopeoffset();
														EXITSCOPESPACE;
													}
				;

funcdef:		funcprefix funcargs funcbody 	{
													EXITSCOPESPACE;
													$$ = $1;
													$$->totallocals = $3;
													int oldoffset = pop(&scopeoffsetstack);
													restorecurrscopeoffset(oldoffset);
													emit_i(funcend, NULL, NULL, lvalue_expr($1), currQuad, yylineno);
												}
				;

const:			INTEGER 	{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'INTEGER'\n"); 
								$$ = newexpr_constnum($1);
							}
				| REAL 		{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'REAL'\n"); 
								$$ = newexpr_constnum($1);
							}
				| STRING 	{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'STRING'\n"); 
								$$ = newexpr_conststring($1);
							}
				| NIL 		{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'NIL'\n"); 
								$$ = newexpr(nil_e);
							}
				| TRUE 		{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'TRUE'\n"); 
								$$ = newexpr_constbool(1);
							}
				| FALSE		{ 	if(debug_print) fprintf(yyout, "Reduced rule 'FALSE'\n"); 
								$$ = newexpr_constbool(0);
							}
				;

idlist:			ID 			{
								func_lvalue = 0;
								if(last_type == USERFUNC_T){
									if( ( (scope_node = lookup(scopelist, curr_scope, yytext)) != NULL) &&
										(scope_node->symbol->isActive == 1)
										){
										if(scope_node->symbol->type == LIBFUNC_T){
											tmp = strdup("error: collision with library function: ");
											strcat(tmp,yytext);
											yyerror(tmp);
											return -1;
										} else if (getSymScope(scope_node->symbol) == curr_scope) {
											yyerror("error: variable already defined: ");
											return -1;
										}
									}
									(insertArgsFunction(symTable, &scopelist, last_node, yytext, curr_scope, yylineno))->offset = currscopeoffset();
									inccurrscopeoffset();
								}

							} 
				idlists 	{ 
								if(debug_print) fprintf(yyout, "Reduced rule 'ID idlists'\n"); 
							}
				|
				;

idlists:		idlists COMMA ID 	{
										func_lvalue = 0;
										if(last_type == USERFUNC_T){
											if( ( (scope_node = lookup(scopelist, curr_scope, yytext)) != NULL) &&
												(scope_node->symbol->isActive == 1)
												){
												if(scope_node->symbol->type == LIBFUNC_T){
													tmp = strdup("error: collision with library function: ");
													strcat(tmp,yytext);
													yyerror(tmp);
													return -1;
												} else if (getSymScope(scope_node->symbol) == curr_scope) {
													yyerror("error: variable already defined: ");
													return -1;
												}
											}
											(insertArgsFunction(symTable, &scopelist, last_node, yytext, curr_scope, yylineno))->offset = currscopeoffset();
											inccurrscopeoffset();
										}
									}
				|
				;

ifprefix:		IF LPARENTH expr RPARENTH			{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'if ( expr )'\n"); 
														backpatch($3->truelist, NEXTQUAD);
														if($3->type == boolexpr_e){
															$$ = $3->falselist;
														} else {
															backpatch($3->falselist, NEXTQUAD+1);
															emit_i(if_eq, $3, newexpr_constbool(1), NULL, NEXTQUAD+2, yylineno);
															$$ = makelist(NEXTQUAD);
															emit_i(jump, NULL, NULL, NULL, -1, yylineno);
														}
													}
				;

elseprefix:		ELSE 								{ 	
														if(debug_print) fprintf(yyout, "Reduced rule 'else'\n"); 
														$$ = NEXTQUAD;
														emit_i(jump, NULL, NULL, NULL, -1, yylineno);
													}
				;

ifstmt:			ifprefix stmt %prec IFX				{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'if ( expr ) stmt'\n");
														backpatch($1, NEXTQUAD);
													}
				| ifprefix stmt elseprefix stmt 	{ 
														if(debug_print) fprintf(yyout, "Reduced rule 'if ( expr ) stmt else stmt'\n"); 
														backpatch($1, $3+1);
														patchlabel($3, NEXTQUAD);
													}
				;

loopstart:		{
					++loopcounter;
				}
				;

loopend:		{
					--loopcounter;
				}
				;

loopstmt:		loopstart stmt loopend 	{
											if(debug_print) fprintf(yyout, "Reduced rule 'loopstart stmt loopend'\n");
										}
				;

whilestart:		WHILE 	{
							if(debug_print) fprintf(yyout, "Reduced rule 'whilestart'\n");
							push(&loopcounterstack, curr_scope);
							LIST_INSERT(statementscounterlist, STMTSTATEMENT, stmtStruct, ADD_2_FRONT);
							$$ = NEXTQUAD;
						}
				;

whilecond:		LPARENTH expr RPARENTH {
											if(debug_print) fprintf(yyout, "Reduced rule 'whilecond'\n");
											if($2->type == boolexpr_e){
												backpatch($2->truelist, NEXTQUAD+1);
												backpatch($2->falselist, NEXTQUAD);
											} else {
												emit_i(if_eq, $2, newexpr_constbool(1), NULL, NEXTQUAD+2, yylineno);
											}
											$$ = NEXTQUAD;
											emit_i(jump, NULL, NULL, NULL, -1, yylineno);
										}

whilestmt:		whilestart whilecond loopstmt	{ 		
													if(debug_print) fprintf(yyout, "Reduced rule 'whilestart whilecond loopstmt'\n");
													pop(&loopcounterstack);
													emit_i(jump, NULL, NULL, NULL, $1, yylineno);
													patchlabel($2, NEXTQUAD);
													if(statementscounterlist){
														breakElem *tmp;
														contElem *tmp2;
														for(tmp = statementscounterlist->breaklist; tmp; tmp = tmp->next){
															patchlabel(tmp->quad, currQuad);
														}
														for(tmp2 = statementscounterlist->contlist; tmp2; tmp2 = tmp2->next){
															patchlabel(tmp2->quad, $1);
														}
														statementscounterlist = deletestmt(statementscounterlist);
													}
												}
				;

N:	{
		$$ = NEXTQUAD;
		emit_i(jump, NULL, NULL, NULL, -1, yylineno);
	}
	;

M:	{
		$$ = NEXTQUAD;
		LIST_INSERT(statementscounterlist, STMTSTATEMENT, stmtStruct, ADD_2_FRONT);
	}
	;

K:	{
		$$ = NEXTQUAD;
	}

forprefix:		FOR 											{
																	push(&loopcounterstack, curr_scope);
																}
				LPARENTH elist SEMICOLON M expr SEMICOLON 		{
																	$$ = (forStruct *) malloc (sizeof(forStruct));
																	$$->test = $6;													
																	if($7->type == boolexpr_e){
																		$$->truelist = $7->truelist;
																		$$->falselist = $7->falselist;
																		$$->enter = NEXTQUAD;	
																	} else {
																		emit_i(if_eq, $7, newexpr_constbool(1), NULL, -1, yylineno);
																	}
																	$$->enter = NEXTQUAD;
																}
				;

forstmt:		forprefix N elist RPARENTH N loopstmt N 	{ 
																if(debug_print) fprintf(yyout, "Reduced rule 'for ( elist ; expr ; elist ) stmt'\n");
																pop(&loopcounterstack);
																if($1->truelist && $1->falselist){
																	backpatch($1->truelist, $2);
																	backpatch($1->falselist, $7+1);
																	patchlabel($2, $5+1);
																} else {
																	patchlabel($1->enter-1, $5+1);
																	patchlabel($2, $7+1);
																}
																patchlabel($5, $1->test);
																patchlabel($7, $2+1);
																if(statementscounterlist){
																	breakElem *tmp;
																	contElem *tmp2;
																	for(tmp = statementscounterlist->breaklist; tmp; tmp = tmp->next){
																		patchlabel(tmp->quad, currQuad);
																	}
																	for(tmp2 = statementscounterlist->contlist; tmp2; tmp2 = tmp2->next){
																		patchlabel(tmp2->quad, $2+1);
																	}
																	statementscounterlist = deletestmt(statementscounterlist);
																}
															}
				;

returnstmt:		RETURN SEMICOLON				{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'return ;'\n");
													if(isEmpty(functioncounterstack)){
														tmp = strdup("error: cannot return outside of function");
														yyerror(tmp);
														return -1;
													}
													emit_i(ret, NULL, NULL, NULL, currQuad, yylineno);
												}
				| RETURN expr SEMICOLON			{ 
													if(debug_print) fprintf(yyout, "Reduced rule 'return expr ;'\n"); 
													if(isEmpty(functioncounterstack)){
														tmp = strdup("error: cannot return outside of function");
														yyerror(tmp);
														return -1;
													} 
													emit_i(ret, NULL, NULL, $2, currQuad, yylineno);
												}
				;
%%

int yyerror (char* yaccProvidedMessage){
		if(debug_print) fprintf(yyout, "%s: before token: '%s' at line %d\n", yaccProvidedMessage, yytext, yylineno);
		return -1;
}

int main(int argc, char** argv){
	FILE *fp;
	yyout = stdout;
	symTable = (SymTable_T *) SymTable_new();

	last_node = put(symTable, &scopelist, "print", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "input", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "objectmemberkeys", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "objecttotalmembers", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "objectcopy", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "totalarguments", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "argument", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "typeof", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "strtonum", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "sqrt", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "cos", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;
	last_node = put(symTable, &scopelist, "sin", 0, 0, LIBFUNC_T, libraryfunc_s, functionlocal);
	last_node->scopeSpace = -1;

  	if (argc > 1){
    	if (!(yyin = fopen(argv[1], "r"))){
      		fprintf(stderr, "Cannot read file: %s\n", argv[1]);
      		return 1;
    	}
		if (argc > 2){
			if (!(fp = fopen(argv[2], "w"))){
				fprintf(stderr, "Cannot write file: %ss\n", argv[2]);
				return 1;
			}
			yyout = fp;
		}
  	}else{
    	yyin = stdin;
  	}
	yyparse();
	printHashTable(symTable);
	printQuads();
	generator();
	printInstructions();
	return 1;
}