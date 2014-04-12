rm(list = ls())
library(ggplot2)

for (alg in c("sv","bfs")){
	for (arch in c("arn","hsw","bobcat", "bonnell","pld","slv")) {
	  for (graph in c("astro-ph","audikw1","auto","coAuthorsDBLP","coPapersDBLP","cond-mat-2003", 
	                  "cond-mat-2005","ecology1", "ldoor", "power","preferentialAttachment")){
		perfdata = read.table(paste(arch, "-",alg,"/", graph, ".log", sep=""), sep="\t")
    
		if(alg=="sv"){
		  names(perfdata) <- c("dummy","Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions","Vertices", "Edges")
			perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
		  phase <-"iteration"
      Phase <-"Iteration"
		}
		else
		{
  
 			names(perfdata) <- c("dummy","Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
 			perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
 			phase <-"level"    
      Phase <-"Level"
		}
    
		maxIter<-max(perfdata$Iteration)
    
		timePlot <- 
		  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=Time, colour=Algorithm)) +
		  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
		  ylab("Time (s)") + 
		  ylim(c(0,max(perfdata$Time)))+        
		  ggtitle(paste("Execution time per ",phase))

#		if(alg=="bfs"){
		  
			perfdata$timePerEdge = perfdata$Time / perfdata$Edges
			timePerEdgePlot <- 
			  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=timePerEdge, colour=Algorithm)) +
			  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
			  ylab("Time (ns)") +
        ylim(c(0,max(perfdata$timePerEdge)))+    
			  ggtitle("Averege time per edge travesal")
#		}		
    
		branchesPlot <- 
		  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=Branches, colour=Algorithm)) +
		  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
      ylab("Branches") +
      ylim(c(0,max(perfdata$Branches)))+    
		  ggtitle(paste("Branches per ", phase))

#		if(alg=="bfs"){
		  
			perfdata$BranchesPerEdge = perfdata$Branches / perfdata$Edges
			branchesPerEdgePlot <- 
			  ggplot(subset(perfdata, Iteration!=1),aes(x=Iteration, y=BranchesPerEdge, colour=Algorithm)) +
			  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
			  ylab("Branches") +
        ylim(c(0,max(perfdata$BranchesPerEdge)))+    
			  ggtitle("Average number of branches \n per edge travesal")
#		}
		
		mispredictionsPlot <- 
		  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=Mispredictions, colour=Algorithm)) +
		  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
		  ylab("Branch Mispredictions") +
      ylim(c(0,max(perfdata$Mispredictions)))+    
		  ggtitle(paste("Branch mispredictions per ",phase))
		
#		if(alg=="bfs"){
			perfdata$mispredictionsPerEdge = perfdata$Mispredictions / perfdata$Edges
			mispredictionsPerEdgePlot <- 
			  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=mispredictionsPerEdge, colour=Algorithm)) +
			  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
			  ylab("Branch Mispredictions") +
        ylim(c(0,max(perfdata$mispredictionsPerEdge)))+      
			  ggtitle("Branch mispredictions per \n edge travesal")
#		}		
    
		instructionsPlot <- 
		  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=Instructions, colour=Algorithm)) +
		  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
		  ylab("Instructions") +
      ylim(c(0,max(perfdata$Instructions)))+      
		  ggtitle(paste("Instructions per ",phase))


#		if(alg=="bfs"){	
			perfdata$instructionsPerEdge = perfdata$Instructions / perfdata$Edges
			instructionsPerEdgePlot <- 
			  ggplot(subset(perfdata, Iteration!=0),aes(x=Iteration, y=instructionsPerEdge, colour=Algorithm)) +
			  geom_line() + xlab(Phase) + theme(text = element_text(size=20)) +
			  ylab("Instructions") +
        ylim(c(0,max(perfdata$instructionsPerEdge)))+      
			  ggtitle("Instructions per edge travesal")
#		}
    
		ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-time.pdf", sep=""), timePlot, width=8, height=5, units="in")
  	ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-branches.pdf", sep=""), branchesPlot, width=8, height=5, units="in")
		ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-mispredictions.pdf", sep=""), mispredictionsPlot, width=8, height=5, units="in")
		ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-instructions.pdf", sep=""), instructionsPlot, width=8, height=5, units="in")


#		if(alg=="bfs"){
			ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-timePerEdge.pdf", sep=""), timePerEdgePlot, width=8, height=5, units="in")
			ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-branchesPerEdge.pdf", sep=""), branchesPerEdgePlot, width=8, height=5, units="in")
			ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-mispredictionsPerEdge.pdf", sep=""), mispredictionsPerEdgePlot, width=8, height=5, units="in")
			ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-", alg, "/", graph, "-instructionsPerEdge.pdf", sep=""), instructionsPerEdgePlot, width=8, height=5, units="in")

#		}		
	  
	  }
	}

  
}