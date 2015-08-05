#!/usr/bin/python
import sys
import subprocess


def printGraphList(graphList):
    for g in graphList:
        print g

def main(argv):
    printC=True
    printJava=True
    printPy=True
  
    graphList=[]
    graphList.append(["clustering/","football"])
    graphList.append(["clustering/","astro-ph"])
    graphList.append(["clustering/","caidaRouterLevel"])
    graphList.append(["clustering/","cond-mat-2005"])
    graphList.append(["clustering/","preferentialAttachment"])
    graphList.append(["clustering/","smallworld"])
    graphList.append(["coauthor/","coAuthorsDBLP"])
#     graphList.append(["coauthor/","coPapersDBLP"])  - Failed python and java

    graphList.append(["delaunay/","delaunay_n20"])
    graphList.append(["matrix/","audikw1"])
    graphList.append(["matrix/","cage15"])
    graphList.append(["matrix/","ldoor"])
    graphList.append(["walshaw/","auto"])
    graphList.append(["walshaw/","finan512"])
    graphList.append(["walshaw/","m14b"])
    graphList.append(["walshaw/","vibrobox"])

    graphListMin=[]
    graphListMin.append(["clustering/" ,"football"])
    graphListMin.append(["clustering/","astro-ph"])


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
				# print temp1
				temp2="--gname="+graph[1]
				# print temp2
				subprocess.call(["python", "cctmain.py", temp1,temp2 ],stdout=outfile)

if __name__ == "__main__":
    main(sys.argv[1:])
                   
