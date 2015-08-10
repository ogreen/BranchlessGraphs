#!/usr/bin/python
import sys
import subprocess
from graphNames import graphList

def printGraphList(graphList):
    for g in graphList:
        print g

def main(argv):
    printC=True
    printJava=True
    printPy=True

#    print graphList
    #graphDir="/home/greenman/data/dimacs/"
    graphDir="/shared/users/greenman/dimacs/"
    for graph in graphList:
        graphName=graphDir+graph[0]+graph[1]+".graph"
        with open("res-titan/"+graph[1]+".csv", "w") as outfile:
            print graphName
            if(printC==True):
                subprocess.call(["./cct", graphName, graph[1]],stdout=outfile)   
            if(printJava==True):
                subprocess.call(["java","cct",graphName,graph[1]],stdout=outfile)
            if(printPy==True):
                temp1="--ifile="+graphName
                temp2="--gname="+graph[1]
                subprocess.call(["python", "cctmain.py", temp1,temp2 ],stdout=outfile)

if __name__ == "__main__":
    main(sys.argv[1:])
                   
