.PHONY:	all clean bench
all: bfs sv cct bc

DEFINES = 
ifeq ($(INTEL_BROADWELL_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_BROADWELL_COUNTERS
endif
ifeq ($(INTEL_HASWELL_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_HASWELL_COUNTERS
endif
ifeq ($(INTEL_IVYBRIDGE_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_IVYBRIDGE_COUNTERS
endif
ifeq ($(INTEL_SILVERMONT_COUNTERS),1)
	DEFINES += -DHAVE_INTEL_SILVERMONT_COUNTERS
endif
ifeq ($(AMD_FAMILY15_COUNTERS),1)
	DEFINES += -DHAVE_AMD_FAMILY15_COUNTERS
endif
ifeq ($(ARM_COUNTERS),1)
	DEFINES += -DHAVE_ARM_COUNTERS
endif
 
ifeq ($(ARCH),arm)
CFLAGS=-marm -DARMASM -march=armv7-a -mfpu=neon-vfpv4 
#CC=clang
graph.o: graph_arm.py
	python $<
	$(CC) -c graph_arm.s -o graph.o
		
else
#march=native
CFLAGS=-DX86 -march=core-avx2 -fverbose-asm -Wimplicit-function-declaration -mavx2 -mfma -ffp-contract=fast
CC=clang
graph.o: graph_x86_64.py
	python $<
	nasm -f elf64 -o graph.o graph_x86_64.s
	
endif

bfs: main.c timer.c bfs.c bfsBU.c graph.o Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_BFS -o $@ main.c timer.c bfs.c bfsBU.c graph.o $(LDFLAGS) -lrt
sv: main.c timer.c sv.c graph.o Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_SV -o $@ main.c timer.c sv.c graph.o $(LDFLAGS) -lrt
cct: main.c timer.c cct.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_CCT -o $@  main.c timer.c cct.c $(LDFLAGS) -lrt
bc: main.c timer.c bc.c bfs.c Makefile
	$(CC) -g -O3 -std=gnu99 $(CFLAGS) $(DEFINES) -Wno-unused-result -DBENCHMARK_BC -o $@  main.c timer.c bc.c bfs.c $(LDFLAGS) -lrt


bench-bfs: bfs
	-mkdir -p $(SYSNAME)-bfs
	./bfs ~/data/dimacs/clustering/astro-ph.graph > $(SYSNAME)-bfs/astro-ph.log
	./bfs ~/data/dimacs/matrix/audikw1.graph > $(SYSNAME)-bfs/audikw1.log
	./bfs ~/data/dimacs/walshaw/auto.graph > $(SYSNAME)-bfs/auto.log
	./bfs ~/data/dimacs/coauthor/coAuthorsDBLP.graph > $(SYSNAME)-bfs/coAuthorsDBLP.log
	./bfs ~/data/dimacs/coauthor/coPapersDBLP.graph > $(SYSNAME)-bfs/coPapersDBLP.log
	./bfs ~/data/dimacs/clustering/cond-mat-2003.graph > $(SYSNAME)-bfs/cond-mat-2003.log
	./bfs ~/data/dimacs/clustering/cond-mat-2005.graph > $(SYSNAME)-bfs/cond-mat-2005.log
	./bfs ~/data/dimacs/matrix/ecology1.graph > $(SYSNAME)-bfs/ecology1.log
#	./bfs ~/data/dimacs/clustering/italy.graph > $(SYSNAME)-bfs/italy.log
#	./bfs ~/data/dimacs/clustering/kron_g500-simple-logn16.graph > $(SYSNAME)-bfs/kron_g500-simple-logn16.log
	./bfs ~/data/dimacs/matrix/ldoor.graph > $(SYSNAME)-bfs/ldoor.log
#	./bfs ~/data/dimacs/clustering/netherlands.graph > $(SYSNAME)-bfs/netherlands.log
	./bfs ~/data/dimacs/clustering/netscience.graph > $(SYSNAME)-bfs/netscience.log
	./bfs ~/data/dimacs/clustering/power.graph > $(SYSNAME)-bfs/power.log
	./bfs ~/data/dimacs/clustering/preferentialAttachment.graph > $(SYSNAME)-bfs/preferentialAttachment.log

bench-sv: sv
	-mkdir -p $(SYSNAME)-sv
	./sv ~/data/dimacs/clustering/astro-ph.graph > $(SYSNAME)-sv/astro-ph.log
	./sv ~/data/dimacs/matrix/audikw1.graph > $(SYSNAME)-sv/audikw1.log
	./sv ~/data/dimacs/walshaw/auto.graph > $(SYSNAME)-sv/auto.log
	./sv ~/data/dimacs/coauthor/coAuthorsDBLP.graph > $(SYSNAME)-sv/coAuthorsDBLP.log
	./sv ~/data/dimacs/coauthor/coPapersDBLP.graph > $(SYSNAME)-sv/coPapersDBLP.log
	./sv ~/data/dimacs/clustering/cond-mat-2003.graph > $(SYSNAME)-sv/cond-mat-2003.log
	./sv ~/data/dimacs/clustering/cond-mat-2005.graph > $(SYSNAME)-sv/cond-mat-2005.log
	./sv ~/data/dimacs/matrix/ecology1.graph > $(SYSNAME)-sv/ecology1.log
#	./sv ~/data/dimacs/clustering/italy.graph > $(SYSNAME)-sv/italy.log
#	./sv ~/data/dimacs/clustering/kron_g500-simple-logn16.graph > $(SYSNAME)-sv/kron_g500-simple-logn16.log
	./sv ~/data/dimacs/matrix/ldoor.graph > $(SYSNAME)-sv/ldoor.log
#	./sv ~/data/dimacs/clustering/netherlands.graph > $(SYSNAME)-sv/netherlands.log
	./sv ~/data/dimacs/clustering/netscience.graph > $(SYSNAME)-sv/netscience.log
	./sv ~/data/dimacs/clustering/power.graph > $(SYSNAME)-sv/power.log
	./sv ~/data/dimacs/clustering/preferentialAttachment.graph > $(SYSNAME)-sv/preferentialAttachment.log

bench-beta: sv
	-mkdir -p $(SYSNAME)-sv
	./sv ~/data/dimacs/matrix/ldoor.graph > $(SYSNAME)-sv/ldoor.log
 
 
bench-cct: cct
	-mkdir -p $(SYSNAME)-sv
	./cct ~/data/dimacs/clustering/astro-ph.graph #> $(SYSNAME)-sv/astro-ph.log
	#./cct ~/data/dimacs/matrix/audikw1.graph #> $(SYSNAME)-sv/audikw1.log
	./cct ~/data/dimacs/walshaw/auto.graph #> $(SYSNAME)-sv/auto.log
	./cct ~/data/dimacs/coauthor/coAuthorsDBLP.graph #> $(SYSNAME)-sv/coAuthorsDBLP.log
	#./cct ~/data/dimacs/coauthor/coPapersDBLP.graph #> $(SYSNAME)-sv/coPapersDBLP.log
	./cct ~/data/dimacs/clustering/cond-mat-2003.graph #> $(SYSNAME)-sv/cond-mat-2003.log
	./cct ~/data/dimacs/clustering/cond-mat-2005.graph #> $(SYSNAME)-sv/cond-mat-2005.log
	./cct ~/data/dimacs/matrix/ecology1.graph #> $(SYSNAME)-sv/ecology1.log
#	./cct ~/data/dimacs/clustering/italy.graph > $(SYSNAME)-sv/italy.log
#	./cct ~/data/dimacs/clustering/kron_g500-simple-logn16.graph > $(SYSNAME)-sv/kron_g500-simple-logn16.log
#   ./cct ~/data/dimacs/matrix/ldoor.graph #> $(SYSNAME)-sv/ldoor.log
#	./cct ~/data/dimacs/clustering/netherlands.graph > $(SYSNAME)-sv/netherlands.log
	./cct ~/data/dimacs/clustering/netscience.graph #> $(SYSNAME)-sv/netscience.log
	./cct ~/data/dimacs/clustering/power.graph #> $(SYSNAME)-sv/power.log
	./cct ~/data/dimacs/clustering/preferentialAttachment.graph #> $(SYSNAME)-sv/preferentialAttachment.log
 
clean:
	-rm -f bfs
	-rm -f sv
	-rm -f cct
	-rm -f bc
	-rm -f *.o
