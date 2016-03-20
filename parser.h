/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ID = 258,
     STRING = 259,
     GREATEREQUAL = 260,
     LESSEQUAL = 261,
     INCRE = 262,
     DECRE = 263,
     TWOPERIOD = 264,
     EQUAL = 265,
     NOTEQUAL = 266,
     ACCESSOR = 267,
     INTEGER = 268,
     REAL = 269,
     FUNCTION = 270,
     TRUE = 271,
     FALSE = 272,
     NIL = 273,
     IF = 274,
     ELSE = 275,
     FOR = 276,
     WHILE = 277,
     RETURN = 278,
     CONTINUE = 279,
     BREAK = 280,
     AND = 281,
     OR = 282,
     NOT = 283,
     LOCAL = 284,
     ASSIGN = 285,
     GREATER = 286,
     LESS = 287,
     PLUS = 288,
     MINUS = 289,
     MOD = 290,
     DIV = 291,
     MULT = 292,
     LBRACE = 293,
     RBRACE = 294,
     LBRACKET = 295,
     RBRACKET = 296,
     LPARENTH = 297,
     RPARENTH = 298,
     SEMICOLON = 299,
     COMMA = 300,
     COLON = 301,
     PERIOD = 302,
     UMINUS = 303,
     IFX = 304
   };
#endif
/* Tokens.  */
#define ID 258
#define STRING 259
#define GREATEREQUAL 260
#define LESSEQUAL 261
#define INCRE 262
#define DECRE 263
#define TWOPERIOD 264
#define EQUAL 265
#define NOTEQUAL 266
#define ACCESSOR 267
#define INTEGER 268
#define REAL 269
#define FUNCTION 270
#define TRUE 271
#define FALSE 272
#define NIL 273
#define IF 274
#define ELSE 275
#define FOR 276
#define WHILE 277
#define RETURN 278
#define CONTINUE 279
#define BREAK 280
#define AND 281
#define OR 282
#define NOT 283
#define LOCAL 284
#define ASSIGN 285
#define GREATER 286
#define LESS 287
#define PLUS 288
#define MINUS 289
#define MOD 290
#define DIV 291
#define MULT 292
#define LBRACE 293
#define RBRACE 294
#define LBRACKET 295
#define RBRACKET 296
#define LPARENTH 297
#define RPARENTH 298
#define SEMICOLON 299
#define COMMA 300
#define COLON 301
#define PERIOD 302
#define UMINUS 303
#define IFX 304




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 30 "parser.y"

	char* 						stringValue;
	int 						intValue;
	double						realValue;
	struct expr					*exprValue;
	struct func 				*funcValue;
	struct SymbolTableEntry 	*symValue; 
	struct forStruct			*forValue;
	struct stmtStruct			*stmtValue;
	struct booleanlist			*boolValue;



/* Line 2068 of yacc.c  */
#line 162 "parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


