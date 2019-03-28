all: buNeDuFork

buNeDuFork: main.o
	gcc main.o -o buNeDuFork

131044019_main.o: main.c
	gcc -c main.c

clean:
	rm *.o buNeDuFork
