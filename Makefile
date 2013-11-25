CFLAGS += -march=native -O3

.PHONY:	all clean
all: bfs

bfs: main.c timer.c bfs.c Makefile
	$(CC) $(CFLAGS) -g -std=gnu99 -o $@ main.c timer.c bfs.c $(LDFLAGS) -lrt

clean:
	-rm -f bfs
	-rm -f *.o
