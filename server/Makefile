############################# Makefile ##########################

all: client srv

# compila o prg de testes do server
client: client_teste.o
	gcc -o client client_teste.o 
client_teste.o: client_teste.c
	gcc -o client_teste.o -c client_teste.c -W -Wall -ansi -pedantic 
# compila o server
srv: frozen.o sqlite3.o srv1.o   
	gcc -o srv frozen.o sqlite3.o srv1.o -lpthread -ldl
frozen.o: frozen.c frozen.h
	gcc -o frozen.o -c frozen.c -lpthread -ldl
sqlite3.o: sqlite3.c sqlite3.h
	gcc -o sqlite3.o -c sqlite3.c -lpthread -ldl
srv1.o: srv1.c frozen.h sqlite3.h
	gcc -o srv1.o -c srv1.c -W -Wall -ansi -lpthread -ldl
clean:
	rm -rf *.o
clearall: clean
	rm -rf srv
	rm -rf client