CC=g++
CFLAGS=-c -Wall

all: bgame

bgame: bgame.o 
	$(CC) bgame.o -o bgame

bgame.o: bgame.cpp
	$(CC) $(CFLAGS) bgame.cpp

clean:
	rm -rf *.o bgame
