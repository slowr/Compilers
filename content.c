#include "content.h"

int startofComment = 1;
int tmpint;
char *tmpchar;
double tmpdouble;
int comment_nesting = 0;

int newState(char c){
	if(c == 'a') return ALARM;
	else if(c == 'b') return BACKSPACE;
	else if(c == 'f') return FORMFEED;
	else if(c == 'n') return NEWLINE;
	else if(c == 'r') return CARIAGE;
	else if(c == 't') return HTAB;
	else if(c == 'v') return VTAB;
	else if(c == '\\') return BACKSLASH;
	else if(c == '\'') return SINGLEQUOT;
	else if(c == '\"') return DOUBLEQUOT;
	else if(c == '?') return QUESTMARK;
	else if(c == '0') return NULLTERM;
	else return -1;
}

int fixString(char *str){
	char *nStr;
	int i,j=0;
	escape_state state = WAIT; 
	nStr = (char *) malloc ((strlen(str)+1)*sizeof(char));
	for(i=0; i<strlen(str); i++){
		switch(state){
			case(WAIT):
				if(str[i] == '\\'){
					state = newState(str[i+1]);
					if(state == -1){
						nStr[j] = '\0';
						free(nStr);
						return -1;
					}
				}else{
					nStr[j++] = str[i];
					state = WAIT;
				}
				break;
			case(ALARM):
				nStr[j++] = '\a';
				state = WAIT;
				break;
			case(BACKSPACE):
				nStr[j++] = '\b';
				state = WAIT;
				break;
			case(FORMFEED):
				nStr[j++] = '\f';
				state = WAIT;
				break;
			case(NEWLINE):
				nStr[j++] = '\n';
				state = WAIT;
				break;
			case(CARIAGE):
				nStr[j++] = '\r';
				state = WAIT;
				break;
			case(HTAB):
				nStr[j++] = '\t';
				state = WAIT;
				break;
			case(VTAB):
				nStr[j++] = '\v';
				state = WAIT;
				break;
			case(BACKSLASH):
				nStr[j++] = '\\';
				state = WAIT;
				break;
			case(SINGLEQUOT):
				nStr[j++] = '\'';
				state = WAIT;
				break;
			case(DOUBLEQUOT):
				nStr[j++] = '\"';
				if((i+1) >= strlen(str)) return -2;
				state = WAIT;
				break;
			case(QUESTMARK):
				nStr[j++] = '\?';
				state = WAIT;
				break;
			case(NULLTERM):
				nStr[j++] = '\0';
				state = WAIT;
				break;
		}
	}
	nStr[j] = '\0';
	memset(str,0,strlen(str));
	memcpy(str,nStr,j);
	free(nStr);
	return 0;
}
