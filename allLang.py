#!/usr/bin/python
import sys
import subprocess
from graphNames import graphList
#from plotter import resDir

def printGraphList(graphList):
    for g in graphList:
        print g

def main(argv):
    printC=True
    printJava=True
    printPy=True
    
    benchMarkRealData=True;
    benchMarkSynthetic=True;

#    print graphList
    graphDir="/shared/users/greenman/dimacs/"
    resDir="res-has/"    
    #graphDir="/shared/users/greenman/dimacs/"
    if(benchMarkRealData):
        for graph in graphList:
            graphName=graphDir+graph[0]+graph[1]+".graph"
            with open(resDir+graph[1]+".csv", "w") as outfile:
                print graphName
                if(printC==True):
                    subprocess.call(["./cct", graphName, graph[1],str(0),str(0)],stdout=outfile)   
                if(printJava==True):
                    subprocess.call(["java","cct",graphName,graph[1],str(0),str(0)],stdout=outfile)
                if(printPy==True):
                    temp1="--ifile="+graphName
                    temp2="--gname="+graph[1]
                    temp3="--synthetic=0"
                    subprocess.call(["python", "cctmain.py", temp1,temp2,temp3 ],stdout=outfile)

    if(benchMarkSynthetic):
        graph=[None]*2
        graph[0]="coauthor/"; graph[1]="coAuthorsDBLP";
        graphName=graphDir+graph[0]+graph[1]+".graph"
        synSize=str(10000000);        
        
        with open(resDir+"synthetic" +".csv", "w") as outfile:
            if(printC==True):
                subprocess.call(["./cct", graphName, graph[1],str(1), synSize ],stdout=outfile)   
            if(printJava==True):
                subprocess.call(["java","cct",graphName,graph[1],str(1), synSize],stdout=outfile)
            if(printPy==True):
                temp1="--ifile="+graphName
                temp2="--gname="+graph[1]
                temp3="--synthetic=1"
                temp4="--size="+str(synSize)
                subprocess.call(["python", "cctmain.py", temp1,temp2,temp3,temp4 ],stdout=outfile)
                

if __name__ == "__main__":
    main(sys.argv[1:])
                   
