all: block coordinator

coordinator: coordinator.c boardgen.c
	gcc -Wall -o coordinator coordinator.c
run:
	./coordinator
	
block: block.c
	gcc -Wall -o block block.c

clean:
	rm -f block coordinator
