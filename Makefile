.PHONY:	all clean
all: bfs

bfs: main.c timer.c bfs.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -o $@ main.c timer.c bfs.c $(LDFLAGS) -lrt

clean:
	-rm -f bfs
	-rm -f *.o
