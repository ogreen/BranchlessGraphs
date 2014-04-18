.PHONY:	all clean bench
all: bfs sv

DEFINES = 
ifeq ($(INTEL_HASWELL_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_HASWELL_COUNTERS
elifeq ($(INTEL_IVYBRIDGE_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_IVYBRIDGE_COUNTERS
elifeq ($(INTEL_SILVERMONT_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_SILVERMONT_COUNTERS
endif

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
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c bfsBU.c graph.o $(LDFLAGS) -lrt
sv: main.c timer.c sv.c graph.o Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_SV -o $@ main.c timer.c sv.c graph.o $(LDFLAGS) -lrt


bench-bfs: bfs
	-mkdir -p $(SYSNAME)-bfs
	./bfs ../../marat/Bgraphs/astro-ph.graph > $(SYSNAME)-bfs/astro-ph.log
	./bfs ../../marat/Bgraphs/audikw1.graph > $(SYSNAME)-bfs/audikw1.log
	./bfs ../../marat/Bgraphs/auto.graph > $(SYSNAME)-bfs/auto.log
	./bfs ../../marat/Bgraphs/coAuthorsDBLP.graph > $(SYSNAME)-bfs/coAuthorsDBLP.log
	./bfs ../../marat/Bgraphs/coPapersDBLP.graph > $(SYSNAME)-bfs/coPapersDBLP.log
	./bfs ../../marat/Bgraphs/cond-mat-2003.graph > $(SYSNAME)-bfs/cond-mat-2003.log
	./bfs ../../marat/Bgraphs/cond-mat-2005.graph > $(SYSNAME)-bfs/cond-mat-2005.log
	./bfs ../../marat/Bgraphs/ecology1.graph > $(SYSNAME)-bfs/ecology1.log
#	./bfs ../../marat/Bgraphs/italy.graph > $(SYSNAME)-bfs/italy.log
#	./bfs ../../marat/Bgraphs/kron_g500-simple-logn16.graph > $(SYSNAME)-bfs/kron_g500-simple-logn16.log
	./bfs ../../marat/Bgraphs/ldoor.graph > $(SYSNAME)-bfs/ldoor.log
#	./bfs ../../marat/Bgraphs/netherlands.graph > $(SYSNAME)-bfs/netherlands.log
	./bfs ../../marat/Bgraphs/netscience.graph > $(SYSNAME)-bfs/netscience.log
	./bfs ../../marat/Bgraphs/power.graph > $(SYSNAME)-bfs/power.log
	./bfs ../../marat/Bgraphs/preferentialAttachment.graph > $(SYSNAME)-bfs/preferentialAttachment.log

bench-sv: sv
	-mkdir -p $(SYSNAME)-sv
	./sv ../../marat/Bgraphs/astro-ph.graph > $(SYSNAME)-sv/astro-ph.log
	./sv ../../marat/Bgraphs/audikw1.graph > $(SYSNAME)-sv/audikw1.log
	./sv ../../marat/Bgraphs/auto.graph > $(SYSNAME)-sv/auto.log
	./sv ../../marat/Bgraphs/coAuthorsDBLP.graph > $(SYSNAME)-sv/coAuthorsDBLP.log
	./sv ../../marat/Bgraphs/coPapersDBLP.graph > $(SYSNAME)-sv/coPapersDBLP.log
	./sv ../../marat/Bgraphs/cond-mat-2003.graph > $(SYSNAME)-sv/cond-mat-2003.log
	./sv ../../marat/Bgraphs/cond-mat-2005.graph > $(SYSNAME)-sv/cond-mat-2005.log
	./sv ../../marat/Bgraphs/ecology1.graph > $(SYSNAME)-sv/ecology1.log
#	./sv ../../marat/Bgraphs/italy.graph > $(SYSNAME)-sv/italy.log
#	./sv ../../marat/Bgraphs/kron_g500-simple-logn16.graph > $(SYSNAME)-sv/kron_g500-simple-logn16.log
	./sv ../../marat/Bgraphs/ldoor.graph > $(SYSNAME)-sv/ldoor.log
#	./sv ../../marat/Bgraphs/netherlands.graph > $(SYSNAME)-sv/netherlands.log
	./sv ../../marat/Bgraphs/netscience.graph > $(SYSNAME)-sv/netscience.log
	./sv ../../marat/Bgraphs/power.graph > $(SYSNAME)-sv/power.log
	./sv ../../marat/Bgraphs/preferentialAttachment.graph > $(SYSNAME)-sv/preferentialAttachment.log


clean:
	-rm -f bfs
	-rm -f sv
	-rm -f *.o
