all: virtual compiler
virtual: object
	gcc -g -o avm symtable.o intermediateCode.o stack.o targetCode.o avmCode.o -lm
compiler: yacc flex
	gcc -g -o cmp symtable.o content.o intermediateCode.o stack.o targetCode.o scanner.c parser.c -lm
object:
	gcc -c -g -o stack.o stack.c
	gcc -c -g -o intermediateCode.o intermediateCode.c
	gcc -c -g -o symtable.o symtable.c
	gcc -c -g -o targetCode.o targetCode.c
	gcc -c -g -o avmCode.o avmCode.c
yacc:
	yacc -v -d -g -o parser.c parser.y
flex:
	gcc -c -g -o stack.o stack.c
	gcc -c -g -o intermediateCode.o intermediateCode.c
	gcc -c -g -o symtable.o symtable.c
	gcc -c -g -o content.o content.c
	gcc -c -g -o targetCode.o targetCode.c
	flex scanner.l
clean:
	rm -rf cmp avm
	rm -rf *.o
	rm -rf TargetFile.abc
	rm -rf *.dot *.output
	rm -rf parser.c scanner.c
