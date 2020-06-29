COPTS=-Wall -pedantic -g

all : mytest
	./mytest 

mytest : mypdf.o main.o
	gcc -o mytest mypdf.o main.o -g

main.o : main.c mypdf.h
	gcc -c main.c $(COPTS)

mypdf.o : mypdf.c mypdf.h
	gcc -c mypdf.c $(COPTS)
