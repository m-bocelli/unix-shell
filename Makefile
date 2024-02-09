# choose your compiler
CC=gcc
#CC=gcc -Wall

all: mysh 
mysh: sh.o get_path.o main.c 
	$(CC) -g main.c sh.o get_path.o -o mysh

sh.o: sh.c sh.h
	$(CC) -g -c sh.c

get_path.o: get_path.c get_path.h
	$(CC) -g -c get_path.c

clean:
	rm -rf *.o *.o mysh 
