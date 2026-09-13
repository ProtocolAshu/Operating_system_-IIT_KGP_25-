CC = gcc
CFLAGS = -Wall

# Default target
run: demandpaging
	./demandpaging
	cat output.txt

# Verbose mode target
vrun: demandpaging-verbose
	./demandpaging-verbose
	cat verboseoutput.txt

# Compile normal version
demandpaging: demandpaging.c
	$(CC) $(CFLAGS) -o demandpaging demandpaging.c

# Compile verbose version
demandpaging-verbose: demandpaging.c
	$(CC) $(CFLAGS) -DVERBOSE -o demandpaging-verbose demandpaging.c

# Generate search input
db: gensearch.c
	$(CC) $(CFLAGS) -o gensearch gensearch.c
	./gensearch 128 64

# Clean up generated files
clean:
	rm -f demandpaging demandpaging-verbose gensearch output.txt verboseoutput.txt