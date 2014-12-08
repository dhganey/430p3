all: pj03

pj03: main.o LibDisk.o LibFS.o
	g++ main.o LibDisk.o LibFS.o -o pj03 -Wno-write-strings

main.o: main.cc
	g++ -std=c++11 -c main.cc -Wno-write-strings

LibDisk.o: LibDisk.cc LibDisk.h
	g++ -std=c++11 -c LibDisk.cc LibDisk.h -Wno-write-strings

LibFS.o: LibFS.cc LibFS.h
	g++ -std=c++11 -c LibFS.cc LibFS.h -Wno-write-strings

clean:
	rm *.o
	rm *.out
	rm *.gch
	rm pj03
