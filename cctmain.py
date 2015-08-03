#!/usr/bin/python
import sys, getopt
import time

def readGraphDIMACS(filePath):
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

	return out;
	
def intersectionCountMemOps (alen, apos,blen, bpos ,ind):
	ka=kb=out=counter=0
	if(alen==0 or blen==0 or ind[apos+alen-1]<ind[bpos] or ind[bpos + blen-1]<ind[apos]):
		return counter
	while (1):
		if(ka>=alen or kb>=blen):
			break;
		if(ind[apos+ka]==ind[bpos+kb]):
			ka+=1;kb+=1;out+=1;
			counter+=2;
		elif(ind[apos+ka]<ind[bpos+kb]):
			counter+=1;
			ka+=1;	
		else:
			counter+=1;
			kb+=1;	
	return counter;
	

def intersectionLogMemOps (alen, apos,blen, bpos ,ind,memOps,globalPos):
	ka=kb=out=0
	counter=globalPos
	if(alen==0 or blen==0 or ind[apos+alen-1]<ind[bpos] or ind[bpos + blen-1]<ind[apos]):
		return counter
	while (1):
		if(ka>=alen or kb>=blen):
			break;
		if(ind[apos+ka]==ind[bpos+kb]):
			memOps[counter]=apos+ka;
			memOps[counter+1]=bpos+kb;
			counter+=2;
			ka+=1;kb+=1;out+=1;
		elif(ind[apos+ka]<ind[bpos+kb]):
			memOps[counter]=apos+ka;
			counter+=1;
			ka+=1;	
		else:
			memOps[counter]=bpos+kb;
			counter+=1;
			kb+=1;	
	return counter;
	


def triangleCount(nv, off, ind, triNE, printOutput):
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
	if (printOutput):
		print "Branch-Based     : ", sumBB
		print "Branch-Avoiding  : ", sumBA
		print "TimeBB			: ", totalBB
		print "TimeBA			: ", totalBA

	return sumBB

def ResetVertexAccess(vertexAccess, ne):
	for e in range(0,ne):
		vertexAccess[e]=0;


def printNormalized(readTime,baseTime,  timeList):
	for t in timeList:
#		print( (t-readTime)/baseTime)
#		sys.stdout.write( str((t-readTime)/baseTime) + ',' )
		if (baseTime==0):
			print "Normalizing time unsucessful due to short baseTime"
			return
		sys.stdout.write( "{:.5f}".format((t-readTime)/baseTime)  + ',' )

	print 
def benchMarkCCT(nv,ne, off, ind, triNE):
	
	totalMemOps=0;	
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		actualTriangles=0;
		for iter in range (off[src], off[src+1]):
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];	
			totalMemOps+=intersectionCountMemOps (srcLen, off[src],destLen, off[dest] ,ind)

	print "MemOps           : ", totalMemOps
	pos=0;
	memOps=[None]*totalMemOps;
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		actualTriangles=0;
		for iter in range (off[src], off[src+1]):
			dest=ind[iter];
			destLen=off[dest+1]-off[dest];	
			pos=intersectionLogMemOps (srcLen, off[src],destLen, off[dest] ,ind,memOps,pos)
	
	vertexAccess=[None]*ne;	

#	for i in range(0,1000):
#		sys.stdout.write(str(memOps[i])+" ,")

	start=time.clock()
	ResetVertexAccess(vertexAccess,ne)
	timeSet=time.clock()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.clock()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=1;	
	timeInc1=time.clock()-start
 
	ResetVertexAccess(vertexAccess,ne)
	a=0
	start=time.clock()
	for m in range(0,totalMemOps):
		a+=1;	
	timeInc1NoStore=time.clock()-start
 
	ResetVertexAccess(vertexAccess,ne)
	start=time.clock()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=255;	
	timeAdd255=time.clock()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.clock()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=1000000;	
	timeAdd1M=time.clock()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.clock()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=(vertexAccess[memOps[m]]==0);	
	timeAddCond=time.clock()-start

	ResetVertexAccess(vertexAccess,ne)
	a=b=c=0
	start=time.clock()
	for m in range(0,totalMemOps):
		a+=(vertexAccess[memOps[m]]==0);	
		b+=(vertexAccess[memOps[m]]<=0);	
		c+=(vertexAccess[memOps[m]]>=0);	
	timeAdd3Cond=time.clock()-start
	nediv2=int(0.7*ne);
	ResetVertexAccess(vertexAccess,ne)
	a=b=c=0
	start=time.clock()
	for m in range(0,totalMemOps):
		if(memOps[m]>=nediv2):
			vertexAccess[memOps[m]]=memOps[m];
		else:
			vertexAccess[memOps[m]]=memOps[m];

	timeNVDiv2=time.clock()-start


	timeList=[]
#	timeList.append(timeSet);
	timeList.append(timeInc1);
	timeList.append(timeInc1NoStore);
	timeList.append(timeAdd255);
	timeList.append(timeAdd1M);
	timeList.append(timeAddCond);
	timeList.append(timeAdd3Cond);
	timeList.append(timeNVDiv2	);

	print "Time-set         : ", timeSet	
	printNormalized(timeSet, timeInc1-timeSet,timeList)

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
	triangleCount(nv, off, ind, triNE,True)
	benchMarkCCT(nv, ne, off, ind, triNE)
  

#	benchMarkInstructions(4, 10000);

if __name__ == "__main__":
	main(sys.argv[1:])

