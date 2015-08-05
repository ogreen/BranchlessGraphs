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
	


def timeAlgorithms(nv, off, ind):
	sumBB=sumBA=countFaster=intersections=0;
	totalBB=totalBA=iterBB=iterBA=0.0;
	

	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
#		possibleTriangles=srcLen*srcLen;
#		globalPossibleTriangles+=possibleTriangles;
		for iter1 in range (off[src], off[src+1]):
			intersections+=1
			start= time.time()
			dest=ind[iter1];
			destLen=off[dest+1]-off[dest];	
			sumBA+=intersectionBranchAvoiding (srcLen, off[src],destLen, off[dest] ,ind)			
			end=time.time()
			iterBA=end
			totalBA+=end-start

			start= time.time()	
			dest=ind[iter1];
			destLen=off[dest+1]-off[dest];	
			sumBB+=intersectionBranchBased (srcLen, off[src],destLen, off[dest] ,ind)
			end=time.time()
			iterBB=end
			totalBB+=end-start
			
			if(iterBA<iterBB):
				countFaster+=1

	return totalBA,totalBB,intersections,countFaster, float(countFaster)/float(intersections)


def ResetVertexAccess(vertexAccess, ne):
	for e in range(0,ne):
		vertexAccess[e]=0;

def prettyPrint(nv, ne, timeBA,timeBB,intersections,countFaster, ratioBAWins, timeMem,timeList, graphName):

	printStr = "";
	printStr = printStr + "{:>25s}, ".format(graphName)		
	printStr = printStr + "{:>8s}, ".format("Python")		
	printStr = printStr + "{:8d}, ".format(nv)	
	printStr = printStr + "{:8d}, ".format(ne)	
	printStr = printStr + "{:8d}, ".format(intersections)		
	printStr = printStr + "{:8d}, ".format(countFaster)	
	printStr = printStr + "{:.5f}, ".format(timeBA)
	printStr = printStr + "{:.5f}, ".format(timeBB)
	printStr = printStr + "{:.5f}, ".format(ratioBAWins)

	baseTime=timeList[0]-timeMem
	if (baseTime==0):
		print "Normalizing time unsucessful due to short baseTime"
		return

	for t in timeList:
		printStr = printStr + "{:.5f}, ".format((t-timeMem)/baseTime) 

	print printStr

#	printStr = printStr + "{:.5f}".format((t-readTime)/baseTime)  + ','
	
#	printStr = printStr + "{:.5f}".format((t-readTime)/baseTime)  + ','

# 	if (printStats.cctTimers[CCT_TT_BA]==0){
# 		printf("Normalizing time unsucessful due to short baseTime - Graph probably too small\n");
# 		return;
# 	}
# 
# 	double memTime=printStats.cctTimers[CCT_TT_MEM_ONLY];
# 	double baseTime=printStats.cctTimers[CCT_TT_INC]-memTime;
# 
# 	for(eCCTimers norm=CCT_TT_INC; norm<CCT_TT_LAST; norm++)
# 	{
# 		printf("%.5lf,\t", ((printStats.cctTimers[norm]-memTime)/baseTime));
# 
# 	}
# 
# 	printf("\n");
	

# def printNormalized(readTime,baseTime,  timeList):
# 	for t in timeList:
# 		if (baseTime==0):
# 			print "Normalizing time unsucessful due to short baseTime"
# 			return
# 		sys.stdout.write( "{:.5f}".format((t-readTime)/baseTime)  + ',' )
# 
# 	print 
	
def benchMarkCCT(nv,ne, off, ind, triNE):
	
	totalMemOps=0;	
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		for iter1 in range (off[src], off[src+1]):
			dest=ind[iter1];
			destLen=off[dest+1]-off[dest];	
			totalMemOps+=intersectionCountMemOps (srcLen, off[src],destLen, off[dest] ,ind)

#	print "MemOps           : ", totalMemOps
	pos=0;
	memOps=[None]*totalMemOps;
	for src in range(0,nv):
		srcLen=off[src+1]-off[src];
		for iter1 in range (off[src], off[src+1]):
			dest=ind[iter1];
			destLen=off[dest+1]-off[dest];	
			pos=intersectionLogMemOps (srcLen, off[src],destLen, off[dest] ,ind,memOps,pos)
	
	vertexAccess=[None]*ne;	

	start=time.time()
	ResetVertexAccess(vertexAccess,ne)
	timeMem=time.time()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.time()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=1;	
	timeInc1=time.time()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.time()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=1000000;	
	timeAdd1M=time.time()-start

	ResetVertexAccess(vertexAccess,ne)
	start=time.time()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=nv;	
	timeAddVar=time.time()-start


	ResetVertexAccess(vertexAccess,ne)
	start=time.time()
	for m in range(0,totalMemOps):
		vertexAccess[memOps[m]]+=(vertexAccess[memOps[m]]==0);	
	timeAddCond=time.time()-start

	ResetVertexAccess(vertexAccess,ne)
	a=b=c=0
	start=time.time()
	for m in range(0,totalMemOps):
		a+=(vertexAccess[memOps[m]]==0);	
		b+=(vertexAccess[memOps[m]]<=0);	
		c+=(vertexAccess[memOps[m]]>=0);	
	timeAdd3Cond=time.time()-start

# 	nediv2=int(0.7*ne);
# 	ResetVertexAccess(vertexAccess,ne)
# 	a=b=c=0
# 	start=time.time()
# 	for m in range(0,totalMemOps):
# 		if(memOps[m]>=nediv2):
# 			vertexAccess[memOps[m]]=memOps[m];
# 		else:
# 			vertexAccess[memOps[m]]=memOps[m];
# 	timeNVDiv2=time.time()-start


	timeList=[]
#	timeList.append(timeSet);
	timeList.append(timeInc1);
	timeList.append(timeAdd1M);
	timeList.append(timeAddVar);
	timeList.append(timeAddCond);
	timeList.append(timeAdd3Cond);

	return timeMem, timeList

def main(argv):
	graphName=inputfile = ''

	try:
		opts, args = getopt.getopt(argv,"hig:",["ifile=","gname="])
	except getopt.GetoptError as err:
		print str(err)
		print 'Error cctmain.py -i<inputfile> -g<graphName>'
		sys.exit(2)

	for opt, arg in opts:
		if opt == '-h':
			print 'cctmain.py -i <inputfile> -g <graphName>'
			sys.exit()
		elif opt in ("-i", "--ifile"):
			inputfile = arg
		elif opt in ("-g", "--gname"):
			graphName = arg

	nv,ne,ind,off = readGraphDIMACS(inputfile)

	triNE=[None]*ne

	timeBA,timeBB,intersections,countFaster, ratioBAWins = timeAlgorithms(nv, off, ind)
	timeMem,timeList=benchMarkCCT(nv, ne, off, ind, triNE)

	prettyPrint(nv, ne, timeBA,timeBB,intersections,countFaster, ratioBAWins, timeMem,timeList,graphName)

#	benchMarkInstructions(4, 10000);

if __name__ == "__main__":
#	print sys.argv[1]
	main(sys.argv[1:])
	# params=[]
	# for par in sys.argv:
		# params.append(par)
	# main (params[1:])
