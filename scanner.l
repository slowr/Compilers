%{
#ifdef WIN32
#define YY_NO_UNISTD_H
int isatty(int i){ return 0; }
#endif

#define YY_DECL int yylex(void)

#include "content.h"
#include "parser.h"

%}

%option header-file="./scanner.h"
%option outfile="./scanner.c"
%option noyywrap
%option yylineno
%option	case-insensitive

id				[a-zA-Z][a-zA-Z_0-9]*
wrong_string	\"(\\.|[^"])*
string			{wrong_string}\"
comment			"//".*
comment_2	 	"/*"
space			[\r \t\v\b]
newline			"\n"
int				(0[xX][0-9A-Fa-f]*)|([0-9]*)
real			[0-9]*\.[0-9](e|E)?([0-9]|-[0-9])*

key_IF 			"if"
key_ELSE		"else"
key_WHILE		"while"
key_FOR			"for"
key_FUNCTION	"function"
key_RETURN		"return"
key_BREAK		"break"
key_CONTINUE	"continue"
key_AND			"and"
key_NOT 		"not"
key_OR 			"or"
key_LOCAL		"local"
key_TRUE		"true"
key_FALSE		"false"
key_NIL			"nil"

op_ASSIGN		"="
op_PLUS			"+"
op_MINUS		"-"
op_MULT			"*"
op_DIV			"/"
op_MOD			"%"
op_EQUAL 		"=="
op_NEQUAL 		"!="
op_INCRE		"++"
op_DECRE 		"--"
op_GREATER 		">"
op_LESS 		"<"
op_GREATEREQUAL ">="
op_LESSEQUAL	"<="

puns_LBRACE 	"{"
puns_RBRACE 	"}"
puns_LBRACKET	"["
puns_RBRACKET 	"]"
puns_LPARENTH 	"("
puns_RPARENTH	")"
puns_SEMICOLON	";"
puns_COMMA		","
puns_COLON 		":"
puns_ACCESSOR 	"::"
puns_PERIOD 	"."
puns_TWOPERIOD 	".."

all_other		.
error_case		[!@#$&?~`'?]*{id}

%x COMMENT_2
%%

{key_IF}		{
	return IF;	
}

{key_ELSE}		{
	return ELSE;
}

{key_WHILE}		{
	return WHILE;	
}

{key_FOR}		{
	return FOR;	
}

{key_FUNCTION}	{
	return FUNCTION;
}

{key_RETURN}	{
	return RETURN;
}

{key_BREAK}		{
	return BREAK;
}

{key_CONTINUE}	{
	return CONTINUE;
}

{key_AND}		{
	return AND;
}

{key_NOT}		{
	return NOT;
}

{key_OR}		{
	return OR;
}

{key_LOCAL}		{
	return LOCAL;
}

{key_TRUE}		{
	return TRUE;
}

{key_FALSE}		{
	return FALSE;
}

{key_NIL}		{
	return NIL;
}

{op_ASSIGN}			{
	return ASSIGN;
}

{op_PLUS}			{
	return PLUS;
}

{op_MINUS}			{
	return MINUS;
}

{op_MULT}			{
	return MULT;
}

{op_DIV}			{
	return DIV;
}

{op_MOD}			{
	return MOD;
}

{op_EQUAL}			{
	yylval.stringValue = strdup(yytext); return EQUAL;
}

{op_NEQUAL}			{
	yylval.stringValue = strdup(yytext); return NOTEQUAL;
}

{op_INCRE}			{
	yylval.stringValue = strdup(yytext); return INCRE;
}

{op_DECRE}			{
	yylval.stringValue = strdup(yytext); return DECRE;
}

{op_GREATER}		{
	return GREATER;
}

{op_LESS}			{
	return LESS;
}

{op_GREATEREQUAL}	{
	yylval.stringValue = strdup(yytext); return GREATEREQUAL;
}

{op_LESSEQUAL}		{
	yylval.stringValue = strdup(yytext); return LESSEQUAL;
}

{puns_LBRACE}		{
	return LBRACE;
}

{puns_RBRACE}		{
	return RBRACE;
}

{puns_LBRACKET}		{
	return LBRACKET;
}

{puns_RBRACKET}		{
	return RBRACKET;
}

{puns_LPARENTH}		{
	return LPARENTH;
}

{puns_RPARENTH}		{
	return RPARENTH;
}

{puns_SEMICOLON}	{
	return SEMICOLON;
}

{puns_COMMA}		{
	return COMMA;
}

{puns_COLON}		{
	return COLON;
}

{puns_ACCESSOR}		{
	yylval.stringValue = strdup(yytext); return ACCESSOR;
}

{puns_PERIOD}		{
	return PERIOD;
}

{puns_TWOPERIOD}	{
	yylval.stringValue = strdup(yytext); return TWOPERIOD;
}

{id}		{
	yylval.stringValue = strdup(yytext); return ID;
}

{string}	{
	if(fixString(yytext) == 0){
		yylval.stringValue = strdup(yytext); return STRING;
	}
	else if( fixString(yytext) == -2){
		if(debug_print) fprintf(yyout, "UNTERMINATED_STRING error: token '%s' at line %d\n", yytext, yylineno);
		return -1;
	}else{
		if(debug_print) fprintf(yyout, "INVALID_ESCAPED_CHAR error: token '%s' at line %d\n", yytext, yylineno);
		return -1;
	}
}

{wrong_string} {
	if(debug_print) fprintf(yyout, "UNTERMINATED_STRING error: token '%s' at line %d\n", yytext, yylineno);
	return -1;
}

{comment}	

{comment_2}	{
	startofComment = yylineno;
	BEGIN(COMMENT_2); ++comment_nesting;
}

<COMMENT_2>"\n"
<COMMENT_2>[^(*/&\n)]*
<COMMENT_2>"/*"		++comment_nesting;
<COMMENT_2>"*/"		if(--comment_nesting == 0){ 
						BEGIN(INITIAL);
					}
<COMMENT_2><<EOF>> 	BEGIN(INITIAL); if(debug_print) fprintf(yyout, "UNTERMINATED_COMMENT error: token '%s' at line %d\n", yytext, yylineno); return -1;

"*/"		{

}

{space}

{newline}

{int}		{
	if(strlen(yytext) > 2 && yytext[0] == '0' && (yytext[1] == 'x' | yytext[1] == 'X')){
		tmpint = strtol(yytext, NULL, 16);
	}else{
		tmpint = atoi(yytext);
	}
	yylval.intValue = tmpint; return INTEGER;
}

{real}		{
	tmpdouble = atof(yytext);
	yylval.realValue = tmpdouble; return REAL;
}

{all_other}	{
	if(debug_print) fprintf(yyout, "%d:\t\t\"%s\"\t\tUNKNOWN_TOKEN\n", yylineno, yytext); return -1;
}

{error_case}	{
	if(debug_print) fprintf(yyout, "%d:\t\t\"%s\"\t\tUNKNOWN_TOKEN\n", yylineno, yytext); return -1;
}

<<EOF>> 	{
	return -1;
}

%%
