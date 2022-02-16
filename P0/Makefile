main: queue testafila
	gcc -o queue queue.o testafila.o -Wall 

testafila: testafila.c
	gcc -c -o testafila.o testafila.c

queue: queue.c queue.h
	gcc -c -o queue.o queue.c -Wall

clean:
	rm -rf *.o queue
	clear