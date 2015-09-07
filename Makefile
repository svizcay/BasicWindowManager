CC=g++
CFLAGS=-c -Wall -O3
CLIBS=-lxcb -lxcb-util

all: wm coloredWindow

wm: main.o
	$(CC) -o wm main.o $(CLIBS)

main.o: main.cpp
	$(CC) $(CFLAGS) -o main.o main.cpp

coloredWindow: coloredWindow.o
	$(CC) -o coloredWindow coloredWindow.o $(CLIBS)

coloredWindow.o: coloredWindow.cpp
	$(CC) $(CFLAGS) -o coloredWindow.o coloredWindow.cpp
