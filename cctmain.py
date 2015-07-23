#!/usr/bin/python
import sys, getopt
import time

def readGraphDIMACS(filePath):

#, uint32_t** prmoff, uint32_t** prmind, uint32_t* prmnv, uint32_t* prmne){

	fp = open(filePath,"r")
	firstLine = fp.readline().split()
	nv = int(firstLine[0])
	ne = int(firstLine[1])
	nv+=1
	ne*=2
	off=[None]*(nv+1)
	ind=[None]*(ne)

	off[0]=0
	off[1]=0	
	v=1
	e=0	
	for line in fp:
		vAdj=line.split();
		for u in vAdj:
			ind[e]=int(u)
			e+=1;

		off[v+1]=off[v]+len(vAdj)
		v+=1
	fp.close()
	return nv,ne,ind,off;


def intersectionBranchBased (alen, apos,blen, bpos ,ind):
	ka=kb=out=0
	if(alen==0 or blen==0 or ind[apos+alen-1]<ind[bpos] or ind[bpos + blen-1]<ind[apos]):
		return 0
	while (1):
		if(ka>=alen or kb>=blen):
			break;
		if(ind[apos+ka]==ind[bpos+kb]):
			ka+=1;kb+=1;out+=1;
		elif(ind[apos+ka]<ind[bpos+kb]):
			ka+=1;	
		else:
			kb+=1;	
	return out;


def intersectionBranchAvoiding (alen, apos,blen, bpos ,ind):
	ka=kb=out=0
	if(alen==0 or blen==0 or ind[apos+alen-1]<ind[bpos] or ind[bpos + blen-1]<ind[apos]):
		return 0
	while (1):
		if(ka>=alen or kb>=blen):
			break;
		comp   = (ind[apos+ka]-ind[bpos+kb]);
		ka+= (comp<=0);
		kb+= (comp>=0);
		out+= (comp==0);							

		# cond1=(comp==0)
		# cond2=(comp<0)
# #		print cond2, comp
		# ka+= cond2 + cond1
		# kb+= 1-cond2;
		# out+= cond1;					
	return out;
	
	


def triangleCount(nv, off, ind, triNE):
	edge=0;
	sumBB=sumBA=0;
	
	triBA=triBB=actualTriangles=possibleTriangles=globalPossibleTriangles=0;
	whenFaster=totalBB=totalBA=iterBB=iterBA=ccGlobal=0.0;
	countFaster=0;

	start= time.clock()
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		actualTriangles=0;
		possibleTriangles=srcLen*srcLen;
		globalPossibleTriangles+=possibleTriangles;
		for iter in range (off[src], off[src+1]):
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];	
			sumBA+=intersectionBranchAvoiding (srcLen, off[src],destLen, off[dest] ,ind)			
	end=time.clock()
	totalBA=end-start

	start= time.clock()	
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		actualTriangles=0;
		possibleTriangles=srcLen*srcLen;
		globalPossibleTriangles+=possibleTriangles;
		for iter in range (off[src], off[src+1]):
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];	
			sumBB+=intersectionBranchBased (srcLen, off[src],destLen, off[dest] ,ind)
	end=time.clock()
	totalBB=end-start
	
			
	print "Branch-Based     : ", sumBB
	print "Branch-Avoiding  : ", sumBA
	print "TimeBB			: ", totalBB
	print "TimeBA			: ", totalBA
	return sumBB
	
	
def main(argv):
	inputfile = ''
	try:
		opts, args = getopt.getopt(argv,"hi:",["ifile="])
	except getopt.GetoptError:
		print 'cctmain.py -i <inputfile>'
		sys.exit(2)
	for opt, arg in opts:
		if opt == '-h':
			print 'cctmain.py -i <inputfile>'
			sys.exit()
		elif opt in ("-i", "--ifile"):
			inputfile = arg
	print 'Input file is "', inputfile

	nv,ne,ind,off = readGraphDIMACS(inputfile)

	triNE=[None]*ne
	print nv, ne 	
	triangleCount(nv, off, ind, triNE)
  

if __name__ == "__main__":
	main(sys.argv[1:])

