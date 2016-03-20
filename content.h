#ifndef _CONTENT_H_
#define _CONTENT_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "symtable.h"

typedef enum str_state{
	WAIT,
	ALARM,
	BACKSPACE,
	FORMFEED,
	NEWLINE,
	CARIAGE,
	HTAB,
	VTAB,
	BACKSLASH,
	SINGLEQUOT,
	DOUBLEQUOT,
	QUESTMARK,
	NULLTERM
} escape_state;

extern int startofComment;
extern int tmpint;
extern char *tmpchar;
extern double tmpdouble;
extern int comment_nesting;
extern int debug_print;

int newState(char c);

int fixString(char *str);

#endif /* _CONTENT_H_ */
