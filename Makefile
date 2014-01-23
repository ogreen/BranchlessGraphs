.PHONY:	all clean
all: bfs sv

bfs: main.c timer.c bfs.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c $(LDFLAGS) -lrt
sv: main.c timer.c sv.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -DBENCHMARK_SV -o $@ main.c timer.c sv.c $(LDFLAGS) -lrt

clean:
	-rm -f bfs
	-rm -f sv
	-rm -f *.o
