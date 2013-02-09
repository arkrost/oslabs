all: myreverse

myreverse: rope.o myropereverse.c
	gcc rope.o myropereverse.c -o myreverse

rope.o: rope.h rope.c
	gcc -c rope.c -o rope.o
clean:
	rm -rf *.o myreverse