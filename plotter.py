
"""
Bar chart demo with pairs of bars grouped for easy comparison.
"""
import numpy as np
import matplotlib.pyplot as plt
import re 
from graphNames import graphList

allPlat=["C","Java", "Python"]
#plotColors=['MediumSpringGreen', 'RebeccaPurple','RoyalBlue','FireBrick','DarkCyan']
#plotColors=['MediumSpringGreen', 'RebeccaPurple','RoyalBlue','FireBrick','DarkCyan']
plotColors=['MediumSpringGreen', 'Purple','RoyalBlue','FireBrick','DarkCyan', "DarkGray", "DarkGoldenRod"]

syntheticNames=["Increment", "Add Integer", "Add Variable", "Add Condition =", "Add Condition >=", "Add 3 Conditions", "Add Branch"]

#allPlat=["C"]
graphLabelRotation = 60
def parseResultFile(graphResFile):
    fp = open(graphResFile,"r")
    allRes=[]
    for line in fp:
        line=re.sub('[,]',"", line)
        res = line.split()
        newRes=[res[0], res[1]]
        for r in range (2, len(res)):
            newRes.append(float(res[r]))
        
        allRes.append(newRes)
    fp.close()
#    print allRes
    
    return allRes;    

def filterLanguages(selLang,allResults):
    filtered=[]
    for res in allResults:
        for plat in res:
            if (plat[1]==selLang):
                filtered.append(plat)

    return filtered

def filterColumn(column,allResults):
    filtered=[]
    for res in allResults:
        filtered.append(res[column])

    return filtered


def genTimePlots(resDir,results,graphs):
    for lang in allPlat:
        filterByLang=filterLanguages(lang, results)
        timeBB=filterColumn(6, filterByLang)
        timeBA=filterColumn(7, filterByLang)
    
        for t in range (0, len(timeBB)):
            timeBA[t]=timeBA[t]/timeBB[t]
            timeBB[t]=1
    
        fig, ax = plt.subplots()
        index = np.arange(len(timeBA))
        
        bar_width = 0.35
    
        plt.bar(index, timeBB, bar_width,
                      color='Crimson',
                      label='Branch-Based')
    
        plt.bar(index+bar_width, timeBA, bar_width,
                      color='DodgerBlue',
                      label='Branch-Avoiding')
     
        plt.ylabel('Time') 
        plt.title('Normalized time (to Branch-Based)')
        plt.xticks(index,graphs,rotation=graphLabelRotation)
        plt.legend( loc=0,    ncol=2, mode="expand", borderaxespad=0.)    
        plt.ylim(0, 2)
     
        plt.tight_layout()
        
        figName=resDir+"time"+lang
        plt.savefig(figName+".pdf", format="pdf");
        plt.savefig(figName+".png", format="png");
    return

def genCrossLangCompare(resDir,results,graphs,title,ylabel,column,maxy,fileName, dashedLine):
    fig, ax = plt.subplots()
    index = np.arange(len(graphs))
    bar_width = 0.25
    langCounter=0

    for lang in allPlat:
        filterByLang=filterLanguages(lang, results)
        columnVals=filterColumn(column, filterByLang)

        plt.bar(index+bar_width*langCounter, columnVals, bar_width, 
                #color="b",
                      color=plotColors[langCounter],
                      label=lang)
        langCounter+=1

    if (dashedLine>0):
        plt.plot([0,len(index)], [dashedLine,dashedLine],lw=1.2, color='g', linestyle='--')

    plt.ylabel(ylabel) 
    plt.title(title)
    plt.xticks(index,graphs,rotation=graphLabelRotation)
    plt.legend( loc=0,    ncol=3, mode="expand", borderaxespad=0.)    
    plt.ylim(0, maxy)
    
    plt.tight_layout()
    
    figName=resDir+fileName
    plt.savefig(figName+".pdf", format="pdf");
    plt.savefig(figName+".png", format="png");    
    return

def genSyntheticCompare(resDir,results,graphs,title,ylabel,columnmin,columnmax,maxy,fileName, dashedLine,lang):
    fig, ax = plt.subplots()
    index = np.arange(len(graphs))
    bar_width = 1.0/(columnmax-columnmin+1)
    
    counter=0

    for col in range(columnmin,columnmax):  
        filterByLang=filterLanguages(lang, results)
        columnVals=filterColumn(col, filterByLang)

        plt.bar(index+bar_width*counter, columnVals, bar_width, 
                      color=plotColors[counter], label=syntheticNames[counter])
        counter+=1

    if (dashedLine>0):
        plt.plot([0,len(index)], [dashedLine,dashedLine],lw=1.2, color='black', linestyle='--')


    plt.ylabel(ylabel) 
    plt.title(title)
    plt.xticks(index,graphs,rotation=graphLabelRotation)

    plt.legend( loc=0,ncol=3, mode="expand", borderaxespad=0. )    

    plt.ylim(0, maxy)
    
    plt.tight_layout()
    
    figName=resDir+fileName
    plt.savefig(figName+".pdf", format="pdf");
    plt.savefig(figName+".png", format="png");    


    return

def genSyntheticCompareAll(resDir,results,title,ylabel,columnmin,columnmax,maxy,fileName, dashedLine, lang):
    fig, ax = plt.subplots()
#    index = np.arange(len(graphs))
    index = np.arange(3)
    bar_width = 1.0/(columnmax-columnmin+1)
    
    counter=0

    for col in range(columnmin,columnmax):  
        filterByLang=filterLanguages(lang, results)
        
        columnVals=filterColumn(col, filterByLang)

        plt.bar(index+bar_width*counter, columnVals, bar_width, 
                      color=plotColors[counter], label=syntheticNames[counter])
        counter+=1

    if (dashedLine>0):
        plt.plot([0,len(index)], [dashedLine,dashedLine],lw=1.2, color='black', linestyle='--')


    plt.ylabel(ylabel) 
    plt.title(title)
    plt.xticks(index,["Linear","Mix","Random"],rotation=graphLabelRotation, horizontalalignment='left')

    plt.legend( loc=0,ncol=3, mode="expand", borderaxespad=0. )    

    plt.ylim(0, maxy)
    
    plt.tight_layout()
    
    figName=resDir+fileName
    plt.savefig(figName+".pdf", format="pdf");
    plt.savefig(figName+".png", format="png");    


    return


#######
## MAIN
#######        

graphNum=len(graphList)

#print graphNum

graphs = []
results = []
resDir="res-has/"
for graph in graphList:
    graphs.append(graph[1])
    graphResFile=resDir+graph[1]+".csv"
    results.append(parseResultFile(graphResFile))
#print results

filteredC=filterLanguages("C",results)
filteredPython=filterLanguages("Python",results)
filteredJava=filterLanguages("Java",results)

genTimePlots(resDir,results,graphs)
genCrossLangCompare(resDir,results,graphs,"Percentage of intersection whens Branch-Avoiding is faster","Percentage (%)",8,1.2,"ba-wins",0.5)
genCrossLangCompare(resDir,results,graphs,"Normalized execution time in comparison with addition/increment","Normalized Execution Time",14,25,"cond3",1.0)

for lang in allPlat:
    genSyntheticCompare(resDir,results,graphs,"Normalized execution time for synthetic benchmarks using \n triangle counting memory access pattern","Normalized Execution Time",9,16,30,"graphSynthetic"+lang,1.0,lang)


resultssyn = []
graphResFile=resDir+"synthetic.csv"
resultssyn.append(parseResultFile(graphResFile))

for lang in allPlat:
	genSyntheticCompareAll(resDir,resultssyn,"Normalized execution time for synthetic benchmarks","Normalized Execution Time",2,9,20,"synthetic"+lang,1.0,lang)



