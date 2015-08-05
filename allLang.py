#!/usr/bin/python
import sys
import subprocess


def printGraphList(graphList):
    for g in graphList:
        print g

def main(argv):
    printC=True
    printPy=True
    printJava=True
  
    graphList=[]
    graphList.append(["clustering/","football"])
    graphList.append(["clustering/","astro-ph"])
    graphList.append(["clustering/","caidaRouterLevel"])
    graphList.append(["clustering/","cond-mat-2005"])
    graphList.append(["clustering/","preferentialAttachment"])
    graphList.append(["clustering/","smallworld"])

    graphListMin=[]
    graphListMin.append(["clustering/" ,"football"])


#    print graphList
    graphDir="/home/greenman/data/dimacs/"
    for graph in graphList:
        graphName=graphDir+graph[0]+graph[1]+".graph"
        with open("res/"+graph[1]+".csv", "w") as outfile:
    #        print graphName
            if(printC==True):
                subprocess.call(["./cct", graphName],stdout=outfile)    
            if(printJava==True):
                subprocess.call(["java","cct",graphName],stdout=outfile)
            if(printPy==True):
                subprocess.call(["python", "cctmain.py", "-i", graphName],stdout=outfile)

if __name__ == "__main__":
    main(sys.argv[1:])
                   
