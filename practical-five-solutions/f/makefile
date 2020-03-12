SRC = pool.F90 ran2.F90 qsort.F90 mergesort.F90
LFLAGS=-lm
CC=mpif90

all: 
	$(CC) -o mergesort $(SRC) $(LFLAGS)

clean:
	rm -f mergesort *.mod
