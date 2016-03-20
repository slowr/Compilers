#ifndef _LINK_LIST_H_
#define _LINK_LIST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ADD_2_FRONT 1
#define ADD_2_BACK	0

/* 
	h: head of the list
	st: statement for the assignments
	type: type of node for malloc
	style: add node to front or back
*/
#define LIST_INSERT_FRONT(h, st, type)	({ \
	type *newE1; \
	newE1 = (type *) malloc (sizeof(type)); \
	st; \
	if(h == NULL){ \
		newE1->next = NULL;	\
	} else { \
		newE1->next = h; \
	} \
	h = newE1; \
})

#define LIST_INSERT_BACK(h, st, type)	({ \
	type *newE1; \
	newE1 = (type *) malloc (sizeof(type)); \
	st; \
	if(h == NULL){ \
		newE1->next = NULL;	\
		h = newE1; \
	} else { \
		type *tmp; \
		for(tmp = h; tmp->next; tmp=tmp->next); \
		tmp->next = newE1; \
	} \
})

#define LIST_INSERT(h, st, type, style) ({ \
	if(style){ \
		LIST_INSERT_FRONT(h, st, type); \
	}else{ \
		LIST_INSERT_BACK(h, st, type); \
	} \
})

#define STRINGSTATEMENT(str) 	({ \
	newE1->s = strdup(str); \
})

#define NUMBERSTATEMENT(n)	({ \
	newE1->num = n; \
})

#define FUNCTIONSTATEMENT(a,l,n) ({ \
	newE1->address = a; \
	newE1->localSize = l; \
	newE1->id = n; \
})

#define INCJMPSTATEMENT(c,l) ({ \
	newE1->instrNo = c; \
	newE1->iaddress = l; \
})

#define LABELSTATEMENT(l)	({ \
	newE1->label = l; \
})

#define BRCNTSTATEMENT(q)	({ \
	newE1->quad = currQuad; \
})

#define STMTSTATEMENT	({ \
	newE1->contlist = NULL; \
	newE1->breaklist = NULL; \
})	

#endif
