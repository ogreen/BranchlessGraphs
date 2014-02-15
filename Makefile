.PHONY:	all clean
all: bfs sv

bfs: main.c timer.c bfs.c bfsBU.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c bfsBU.c $(LDFLAGS) -lrt
sv: main.c timer.c sv.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_SV -o $@ main.c timer.c sv.c $(LDFLAGS) -lrt

clean:
	-rm -f bfs
	-rm -f sv
	-rm -f *.o
