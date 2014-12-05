all: pj03

pj03: main.o LibDisk.o LibFS.o
	g++ main.o LibDisk.o LibFS.o -o pj03

main.o: main.cc
	g++ -std=c++11 -c main.cc

LibDisk.o: LibDisk.cc LibDisk.h
	g++ -std=c++11 -c LibDisk.cc LibDisk.h

LibFS.o: LibFS.cc LibFS.h
	g++ -std=c++11 -c LibFS.cc LibFS.h

clean:
	rm *.o
	rm *.out
	rm pj03
