all: hw2

hw2: hw2.o
	g++ -pthread -o hw2 hw2.o -lpthread

hw2.o: hw2.cpp
	g++ -pthread -c hw2.cpp

clean:
	rm -f hw2 hw2.o
