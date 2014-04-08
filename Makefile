.PHONY:	all clean bench
all: bfs sv

ifeq ($(ARCH),arm)
graph.o: graph_arm.py
	python $<
	$(CC) -c graph_arm.s -o graph.o
else
graph.o: graph_x86_64.py
	python $<
	nasm -f elf64 -o graph.o graph_x86_64.s
endif

bfs: main.c timer.c bfs.c bfsBU.c graph.o Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c bfsBU.c graph.o $(LDFLAGS) -lrt
sv: main.c timer.c sv.c graph.o Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) -Wno-unused-result -DBENCHMARK_SV -o $@ main.c timer.c sv.c graph.o $(LDFLAGS) -lrt


bench-bfs: bfs
	./bfs ../../marat/Bgraphs/astro-ph.graph > arn-bfs/astro-ph.log
	./bfs ../../marat/Bgraphs/audikw1.graph > arn-bfs/audikw1.log
	./bfs ../../marat/Bgraphs/auto.graph > arn-bfs/auto.log
	./bfs ../../marat/Bgraphs/coAuthorsDBLP.graph > arn-bfs/coAuthorsDBLP.log
	./bfs ../../marat/Bgraphs/coPapersDBLP.graph > arn-bfs/coPapersDBLP.log
	./bfs ../../marat/Bgraphs/cond-mat-2003.graph > arn-bfs/cond-mat-2003.log
	./bfs ../../marat/Bgraphs/cond-mat-2005.graph > arn-bfs/cond-mat-2005.log
	./bfs ../../marat/Bgraphs/ecology1.graph > arn-bfs/ecology1.log
#	./bfs ../../marat/Bgraphs/italy.graph > arn-bfs/italy.log
#	./bfs ../../marat/Bgraphs/kron_g500-simple-logn16.graph > arn-bfs/kron_g500-simple-logn16.log
	./bfs ../../marat/Bgraphs/ldoor.graph > arn-bfs/ldoor.log
#	./bfs ../../marat/Bgraphs/netherlands.graph > arn-bfs/netherlands.log
	./bfs ../../marat/Bgraphs/netscience.graph > arn-bfs/netscience.log
	./bfs ../../marat/Bgraphs/power.graph > arn-bfs/power.log
	./bfs ../../marat/Bgraphs/preferentialAttachment.graph > arn-bfs/preferentialAttachment.log

bench-sv: sv
	./sv ../../marat/Bgraphs/astro-ph.graph > arn-sv/astro-ph.log
	./sv ../../marat/Bgraphs/audikw1.graph > arn-sv/audikw1.log
	./sv ../../marat/Bgraphs/auto.graph > arn-sv/auto.log
	./sv ../../marat/Bgraphs/coAuthorsDBLP.graph > arn-sv/coAuthorsDBLP.log
	./sv ../../marat/Bgraphs/coPapersDBLP.graph > arn-sv/coPapersDBLP.log
	./sv ../../marat/Bgraphs/cond-mat-2003.graph > arn-sv/cond-mat-2003.log
	./sv ../../marat/Bgraphs/cond-mat-2005.graph > arn-sv/cond-mat-2005.log
	./sv ../../marat/Bgraphs/ecology1.graph > arn-sv/ecology1.log
#	./sv ../../marat/Bgraphs/italy.graph > arn-sv/italy.log
#	./sv ../../marat/Bgraphs/kron_g500-simple-logn16.graph > arn-sv/kron_g500-simple-logn16.log
	./sv ../../marat/Bgraphs/ldoor.graph > arn-sv/ldoor.log
#	./sv ../../marat/Bgraphs/netherlands.graph > arn-sv/netherlands.log
	./sv ../../marat/Bgraphs/netscience.graph > arn-sv/netscience.log
	./sv ../../marat/Bgraphs/power.graph > arn-sv/power.log
	./sv ../../marat/Bgraphs/preferentialAttachment.graph > arn-sv/preferentialAttachment.log


clean:
	-rm -f bfs
	-rm -f sv
	-rm -f *.o
