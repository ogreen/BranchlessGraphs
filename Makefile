.PHONY:	all clean bench
all: bfs sv

bfs: main.c timer.c bfs.c bfsBU.c graph_arm.py Makefile
	python graph_arm.py
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c bfsBU.c graph_arm.s $(LDFLAGS) -lrt
sv: main.c timer.c sv.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_SV -o $@ main.c timer.c sv.c graph_arm.s $(LDFLAGS) -lrt

bench-bfs: bfs
	./bfs data/astro-ph.graph > astro-ph.log
	./bfs data/auto.graph > auto.log
	./bfs data/kron_g500-simple-logn16.graph > kron_g500-simple-logn16.log
	./bfs data/preferentialAttachment.graph > preferentialAttachment.log
	./bfs data/ecology1.graph > ecology1.log

bench-sv: sv
	./sv data/astro-ph.graph > astro-ph.log
	./sv data/auto.graph > auto.log
	./sv data/kron_g500-simple-logn16.graph > kron_g500-simple-logn16.log
	./sv data/preferentialAttachment.graph > preferentialAttachment.log
	./sv data/ecology1.graph > ecology1.log


clean:
	-rm -f bfs
	-rm -f sv
	-rm -f *.o
