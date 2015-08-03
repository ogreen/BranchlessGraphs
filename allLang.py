#!/usr/bin/python
import sys, getopt
import time 
import subprocess

def printGraphList(graphList):
    for g in graphList:
	print g

def main(argv):
    printC=True
    printPy=True
    printJava=True
  
    graphList=[]
    graphList.append("clustering/football")
    graphList.append("clustering/astro-ph")
    graphList.append("clustering/caidiRouterLevel.graph")
    graphList.append("clustering/cond-mat-2005")
    graphList.append("clustering/preferentialAttachment")
    graphList.append("clustering/smallworld")

    print graphList
    graphDir="/shared/users/greenman/dimacs/"
    graphName=graphDir+graphList[0]+".graph"
    print graphName
    if(printC==True):
	subprocess.call(["./cct", graphName])	
    if(printPy==True):
	subprocess.call(["python", "cctmain.py", "-i", graphName])
    if(printJava==True):
	subprocess.call(["java","cct",graphName])

if __name__ == "__main__":
	main(sys.argv[1:])
                   
