rm(list = ls())
library(ggplot2)

#for (alg in c("sv", "bfs")){
for (alg in c("bfs", "bfs")){  
	for (arch in c("ivb", "pld", "slv")) {
	  for (graph in c("astro-ph", "auto", "ecology1", "preferentialAttachment")) {
		perfdata = read.table(paste(arch, "-",alg,"/", graph, ".log", sep=""), sep="\t")
    
		if(alg=="sv"){
			names(perfdata) <- c("Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions")
			perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
		
		}
		else
		{
  
 			names(perfdata) <- c("Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
 			perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
		}
		
		timePlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=Time, colour=Algorithm)) +
		  geom_line() +
		  ylab("Time, ms") +
		  ggtitle("Execution Time")

		perfdata$timePerEdge = perfdata$Branches / perfdata$Edges
		timePerEdgePlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=timePerEdge, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branches per edge Travesal") +
		  ggtitle("Retired Branch Instructions per edge Travesal")
		
    
		branchesPlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=Branches, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branches") +
		  ggtitle("Retired Branch Instructions")

		perfdata$BranchesPerEdge = perfdata$Branches / perfdata$Edges
		branchesPerEdgePlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=BranchesPerEdge, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branches per edge Travesal") +
		  ggtitle("Retired Branch Instructions per edge Travesal")
		
    
		mispredictionsPlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=Mispredictions, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branch Mispredictions") +
		  ggtitle("Mispredicted Branch Instructions")
		
    
		perfdata$mispredictionsPerEdge = perfdata$Branches / perfdata$Edges
		mispredictionsPerEdgePlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=mispredictionsPerEdge, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branches per edge Travesal") +
		  ggtitle("Retired Branch Instructions per edge Travesal")
		
    
		instructionsPlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=Instructions, colour=Algorithm)) +
		  geom_line() +
		  ylab("Instructions") +
		  ggtitle("Retired Instructions")
		
		perfdata$instructionsPerEdge = perfdata$Instructions / perfdata$Edges
		instructionsPerEdgePlot <- 
		  ggplot(perfdata, aes(x=Iteration, y=instructionsPerEdge, colour=Algorithm)) +
		  geom_line() +
		  ylab("Branches per edge Travesal") +
		  ggtitle("Retired Branch Instructions per edge Travesal")
		
    
		ggsave(paste(arch, "-", alg, "/", graph, "-time.png", sep=""), timePlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-timePerEdge.png", sep=""), timePerEdgePlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-branches.png", sep=""), branchesPlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-branchesPerEdge.png", sep=""), branchesPerEdgePlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-mispredictions.png", sep=""), mispredictionsPlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-mispredictionsPerEdge.png", sep=""), mispredictionsPerEdgePlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-instructions.png", sep=""), instructionsPlot, width=8, height=5, units="in")
		ggsave(paste(arch, "-", alg, "/", graph, "-instructionsPerEdge.png", sep=""), instructionsPerEdgePlot, width=8, height=5, units="in")
		
	  
	  }
	}

  
}