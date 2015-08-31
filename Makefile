CC=g++
CFLAGS=-c -Wall -O3
CLIBS=-lxcb

all: wm

wm: main.o
	$(CC) -o wm main.o $(CLIBS)

main.o: main.cpp
	$(CC) $(CFLAGS) -o main.o main.cpp
