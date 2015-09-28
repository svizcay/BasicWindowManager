CC=g++
CFLAGS=-c -Wall -O3 -std=c++11
CLIBS=-lxcb -lxcb-composite -lfreeimage

all: wm 

wm: main.o
	$(CC) -o wm main.o $(CLIBS)

main.o: main.cpp
	$(CC) $(CFLAGS) -o main.o main.cpp

coloredWindow: coloredWindow.o
	$(CC) -o coloredWindow coloredWindow.o $(CLIBS)

coloredWindow.o: coloredWindow.cpp
	$(CC) $(CFLAGS) -o coloredWindow.o coloredWindow.cpp

clean:
	rm -rf *.o wm
