SRC = ran2.c sauirrel_queue.c pool.c squirrel-functions.c squirrel.c
LFLAGS=-lm
CC=mpicc

all:
	$(CC) -o squirrel $(SRC) $(LFLAGS)

clean:
	rm -f squirrel
