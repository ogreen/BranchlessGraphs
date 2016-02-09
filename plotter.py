
"""
Bar chart demo with pairs of bars grouped for easy comparison.
"""
import numpy as np
import matplotlib.pyplot as plt

import re 
from graphNames import graphList
from scipy.lib.lapack.calc_lwork import syev

allPlat=["C","Java", "Python"]
#plotColors=['MediumSpringGreen', 'RebeccaPurple','RoyalBlue','FireBrick','DarkCyan']
#plotColors=['MediumSpringGreen', 'RebeccaPurple','RoyalBlue','FireBrick','DarkCyan']
plotColors=['MediumSpringGreen', 'Purple','RoyalBlue','FireBrick','DarkCyan', "DarkGray", "DarkGoldenRod", "Black"]
plotHatches=["", ".", "/","+","o","//","-","*"]

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


def genTimePlots(filePngName,filePdfName,results,graphs, isArm):
    for lang in allPlat:
        filterByLang=filterLanguages(lang, results)
        timeBB=filterColumn(6, filterByLang)
        timeBA=filterColumn(7, filterByLang)
        timeBAC=filterColumn(8, filterByLang)
    
        for t in range (0, len(timeBB)):
            timeBAC[t]=timeBAC[t]/timeBB[t]
            timeBA[t]=timeBA[t]/timeBB[t]
            timeBB[t]=1
    
        fig, ax = plt.subplots()
        index = np.arange(len(timeBA))
        
        bar_width = 0.28
    
        plt.bar(index, timeBB, bar_width,
                      color='Crimson',
                      label='Branch-Based')
    
        plt.bar(index+bar_width, timeBA, bar_width,
                      color='DodgerBlue',
                      label='Branch-Avoiding', hatch="O")
     

        if (isArm==True and lang=="C"):
            plt.bar(index+(2*bar_width), timeBAC, bar_width,
                      color='Purple',
                      label='Branch-Avoiding Conditional', hatch="//")


        plt.plot([0,len(index)], [1,1],lw=1.2, color='g', linestyle='--')
        plt.rc('xtick', labelsize=18) 
        plt.xlabel('Networks',fontsize=18)
        plt.ylabel('Normalized Time',fontsize=18)    
        plt.ylabel('Normalized Time') 
#        plt.title('Normalized time (to Branch-Based)')
        plt.xticks(index,graphs,rotation=graphLabelRotation)
        plt.legend( loc=0,    ncol=2, mode="expand", borderaxespad=0.)    
        plt.ylim(0, 2)
     
        plt.tight_layout()
        plt.savefig(filePngName+lang+".png", format="png");
        plt.savefig(filePdfName+lang+".pdf", format="pdf");
        plt.close()
    return

def genBAWins(filePngName,filePdfName,results,graphs,title,ylabel,column,maxy, dashedLine, isArm,columnCond):
    fig, ax = plt.subplots()
    index = np.arange(len(graphs))
    bar_width = 0.22
    langCounter=0

    for lang in allPlat:
        if(isArm==True and lang!="C"):
            continue
        filterByLang=filterLanguages(lang, results)
        columnVals=filterColumn(column, filterByLang)

        for t in range (0, len(columnVals)):
            columnVals[t]=columnVals[t]*100


        plt.bar(index+bar_width*langCounter, columnVals, bar_width, 
                #color="b",
                      color=plotColors[langCounter],
                      label=lang,
					  hatch=plotHatches[langCounter])
        langCounter+=1
        
        if (isArm==True and lang=="C"):
            columnValsCond=filterColumn(columnCond, filterByLang)
            for t in range (0, len(columnValsCond)):
                columnValsCond[t]=columnValsCond[t]*100
            
            plt.bar(index+bar_width, columnValsCond, bar_width,
                      color=plotColors[langCounter],
                      label='Branch-Avoiding Conditional')
            langCounter+=1

    if (dashedLine>0):
        plt.plot([0,len(index)], [dashedLine,dashedLine],lw=1.2, color='g', linestyle='--')

    plt.ylabel(ylabel) 
    plt.title(title)
    plt.xticks(index,graphs,rotation=graphLabelRotation)
    plt.legend( loc=0,    ncol=3, mode="expand", borderaxespad=0.)    
    plt.ylim(0, maxy)
    
    plt.tight_layout()
    
    plt.savefig(filePngName+".png", format="png");
    plt.savefig(filePdfName+".pdf", format="pdf");
    plt.close()

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
    plt.savefig(filePngName+".png", format="png");
    plt.savefig(filePdfName+".pdf", format="pdf");
    plt.close()


    return

def genSyntheticCompareAll(filePngName,filePdfName,results,title,ylabel,columnmin,columnmax,maxy, dashedLine, lang):
    fig, ax = plt.subplots()

    index = np.arange(3)
    bar_width = 1.0/(columnmax-columnmin+1)
    
    counter=0

    for col in range(columnmin,columnmax):  
        filterByLang=filterLanguages(lang, results)
        
        columnVals=filterColumn(col, filterByLang)

        plt.bar(index+bar_width*counter, columnVals, bar_width, 
                      color=plotColors[counter], label=syntheticNames[counter],hatch=plotHatches[counter])
        counter+=1

    if (dashedLine>0):
        plt.plot([0,len(index)], [dashedLine,dashedLine],lw=1.2, color='black', linestyle='--')


    plt.ylabel(ylabel) 
    plt.title(title)
    plt.xticks(index,["Linear","Mix","Random"],rotation=graphLabelRotation, horizontalalignment='left')

    plt.legend( loc=0,ncol=3, mode="expand", borderaxespad=0. )    

    plt.ylim(0, maxy)
    
    plt.tight_layout()
    
    plt.savefig(filePngName+".png", format="png");
    plt.savefig(filePdfName+".pdf", format="pdf");
    plt.close()


    return


#######
## MAIN
#######        

graphNum=len(graphList)

systems=['has','snb','arn','odr']
systemsFullName=['Haswell','Sandy-Bridge','Cortex-A15','Cortex-A9']

isArmSystem=[False,False,True,True]
#print graphNum


for s in range(0, len(systems)):
    sys=systems[s];
    isArm=isArmSystem[s];

    graphs = [] 
    results = []
    resDir="cct-res/" +sys+"/"
#    sys="haswell"
    for graph in graphList:
        graphs.append(graph[1])
        graphResFile=resDir+graph[1]+".csv"
        results.append(parseResultFile(graphResFile))
    #print results
    
    filteredC=filterLanguages("C",results)
    filteredPython=filterLanguages("Python",results)
    filteredJava=filterLanguages("Java",results)

    fnamepng="figs-png/"+sys+"-cct-"
    fnamepdf="figs-pdf/"+sys+"-cct-"
#    genTimePlots(fnamepng+"time-", fnamepdf+"time-",results,graphs,isArm)
#    genBAWins(fnamepng+"ba-wins-", fnamepdf+"ba-wins",results,graphs,"Percentage of intersections when Branch-Avoiding is faster","Percentage (%)",9,125,50,isArm,10)
    
    
    #genCrossLangCompare(resDir,results,graphs,"Normalized execution time in comparison with addition/increment","Normalized Execution Time",14,25,"cond3",1.0)
    
    #for lang in allPlat:
    #    genSyntheticCompare(resDir,results,graphs,"Normalized execution time for synthetic benchmarks using \n triangle counting memory access pattern","Normalized Execution Time",9,16,30,"graphSynthetic"+lang,1.0,lang)
    
    
    resultssyn = []
    graphResFile=resDir+"synthetic.csv"
    resultssyn.append(parseResultFile(graphResFile))
    
    for lang in allPlat:
        fnamepng="figs-png/"+sys+"-syn-"+lang
        fnamepdf="figs-pdf/"+sys+"-syn-"+lang        
#        genSyntheticCompareAll(fnamepng,fnamepdf,resultssyn,"Normalized execution time for synthetic benchmarks","Normalized Execution Time",2,9,20,1.0,lang)
    


fig, axes = plt.subplots(nrows=4,sharex=True)
fig.set_size_inches(8, 20)
index = np.arange(len(graphList))
plt.gcf().subplots_adjust(bottom=0.1)
bar_width = 0.28

for s in range(0, len(systems)):
    sys=systems[s];
    isArm=isArmSystem[s];
    graphs = [] 
    results = []
    resDir="cct-res/" +sys+"/"
    #    sys="haswell"
    for graph in graphList:
        graphs.append(graph[1])
        graphResFile=resDir+graph[1]+".csv"
        results.append(parseResultFile(graphResFile))

    filterByLang=filterLanguages("C",results)
    timeBB=filterColumn(6, filterByLang)
    timeBA=filterColumn(7, filterByLang)
    timeBAC=filterColumn(8, filterByLang)

    for t in range (0, len(timeBB)):
        timeBAC[t]=timeBAC[t]/timeBB[t]
        timeBA[t]=timeBA[t]/timeBB[t]
        timeBB[t]=1




    axes[s].bar(index, timeBB, bar_width, color='Crimson', label='Branch-Based')
    axes[s].bar(index+bar_width, timeBA, bar_width, color='DodgerBlue', label='Branch-Avoiding', hatch="O")
    if (isArm==True and lang=="C"):
        axes[s].bar(index+(2*bar_width), timeBAC, bar_width, color='Purple', label='Branch-Avoiding Conditional', hatch="//")

    axes[s].set_ylabel("Normalized Time",fontsize=18)
    axes[s].plot([0,len(index)], [1,1],lw=2, color='g', linestyle='--')
    axes[s].legend( loc=0,    ncol=2, mode="expand", borderaxespad=0.)  
    axes[s].set_title(systemsFullName[s],fontsize=18)

plt.setp(axes, xticks=index,ylim=(0,2))

plt.xticks(index,graphs,rotation=graphLabelRotation,fontsize=15)
plt.savefig("all-plats-time.pdf", format="pdf",bbox_inches='tight');
    

plt.close()


fig, axes = plt.subplots(nrows=4,sharex=True)
fig.set_size_inches(8, 20)
index = np.arange(len(graphList))
plt.gcf().subplots_adjust(bottom=0.1)
bar_width = 0.28

#sys=systems[s];
#isArm=isArmSystem[s];
#allResults=4*[None];
for s in range(0, len(systems)):
    sys=systems[s];
    isArm=isArmSystem[s];
    graphs = [] 
    results = []
    resDir="cct-res/" +sys+"/"
    #    sys="haswell"
    for graph in graphList:
        graphs.append(graph[1])
        graphResFile=resDir+graph[1]+".csv"
        results.append(parseResultFile(graphResFile))


    langCounter=0

    for lang in allPlat:
        if(isArm==True and lang!="C"):
            continue
        filterByLang=filterLanguages(lang, results)
        columnVals=filterColumn(9, filterByLang)

        for t in range (0, len(columnVals)):
            columnVals[t]=columnVals[t]*100

        axes[s].bar(index+bar_width*langCounter, columnVals, bar_width, color=plotColors[langCounter],label=lang,hatch=plotHatches[langCounter])
        langCounter+=1
      
        if (isArm==True and lang=="C"):
            columnValsCond=filterColumn(10, filterByLang)
            for t in range (0, len(columnValsCond)):
                columnValsCond[t]=columnValsCond[t]*100
            
            axes[s].bar(index+bar_width, columnValsCond, bar_width,
                      color=plotColors[langCounter],
                      label='Branch-Avoiding Conditional')
            langCounter+=1

        axes[s].plot([0,len(index)], [50,50],lw=1.2, color='g', linestyle='--')


    axes[s].set_ylabel("Percentage (%)",fontsize=18)
    axes[s].set_title(systemsFullName[s],fontsize=18)

plt.setp(axes, xticks=index,ylim=(0,120))

plt.xticks(index,graphs,rotation=graphLabelRotation,fontsize=15)
plt.savefig("all-plats-winners.pdf", format="pdf",bbox_inches='tight');





    

plt.close()




