#!/usr/bin/python
import sys
import subprocess
from graphNames import graphList
from plotter import resDir

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
    graphDir="/home/greenman/data/dimacs/"
    resDir="res-has/"    
    #graphDir="/shared/users/greenman/dimacs/"
    if(benchMarkRealData):
        for graph in graphList:
            graphName=graphDir+graph[0]+graph[1]+".graph"
            with open(resDir+graph[1]+".csv", "w") as outfile:
                print graphName
                if(printC==True):
                    subprocess.call(["./cct", graphName, graph[1]],stdout=outfile)   
                if(printJava==True):
                    subprocess.call(["java","cct",graphName,graph[1]],stdout=outfile)
                if(printPy==True):
                    temp1="--ifile="+graphName
                    temp2="--gname="+graph[1]
                    temp3="--synthetic=0"
                    subprocess.call(["python", "cctmain.py", temp1,temp2,temp3 ],stdout=outfile)

    if(benchMarkSynthetic):
        graph=[None]*2
        graph[0]="clustering/"; graph[1]="astro-ph";
        graphName=graphDir+graph[0]+graph[1]+".graph"
        with open("res-titan/"+"synthetic" +".csv", "w") as outfile:
#            if(printC==True):
#                subprocess.call(["./cct", graphName, graph[1]],stdout=outfile)   
#            if(printJava==True):
#                subprocess.call(["java","cct",graphName,graph[1]],stdout=outfile)
            if(printPy==True):
                temp1="--ifile="+graphName
                temp2="--gname="+graph[1]
                temp3="--synthetic=1"
                subprocess.call(["python", "cctmain.py", temp1,temp2,temp3 ],stdout=outfile)
                

if __name__ == "__main__":
    main(sys.argv[1:])
                   
