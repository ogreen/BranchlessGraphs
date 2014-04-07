rm(list = ls())
library(ggplot2)

source("multiplot.R")

for (arch in c("arn")){# "ivb", "pld", "slv")) {
#arch <- ""
  for (graph in c("astro-ph","audikw1","auto","coAuthorsDBLP","coPapersDBLP","cond-mat-2003", 
                  "cond-mat-2005","ecology1", "ldoor", "netscience", "power","preferentialAttachment")) {
  #for (graph in c("a")) {  
    perfdata = read.table(paste(arch,"-sv/", graph, ".log", sep=""), sep="\t")
    names(perfdata) <- c("Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions")
    perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
    
    timePlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Time,colour=Algorithm)) +
      geom_line() +
      ylab("Time, ms") +
      theme(text = element_text(size=20)) +
      ylim(c(0,max(perfdata$Time)))+  
      ggtitle("Execution Time") 
      timePlot<-timePlot+theme(legend.position="none")

max(perfdata$Branches)
    branchesPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Branches, colour=Algorithm)) +
      geom_line() +
      ylab("Branches") +
      theme(text = element_text(size=20)) +
      ylim(c(0,max(perfdata$Branches)))+  
      ggtitle("Retired Branch Instructions")
      branchesPlot<-branchesPlot+theme(legend.position="none")
    
    
    mispredictionsPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Mispredictions, colour=Algorithm)) +
      geom_line() +
      ylab("Branch Mispredictions") +
      theme(text = element_text(size=20)) +      
      ylim(c(0,max(perfdata$Mispredictions)))+  
      ggtitle("Mispredicted Branch Instructions")
      mispredictionsPlot<-mispredictionsPlot+theme(legend.position="none")
    
    instructionsPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Instructions, colour=Algorithm)) +
      geom_line() +
      ylab("Instructions") +
      theme(text = element_text(size=20)) +    
      ylim(c(0,max(perfdata$Instructions)))+  
      ggtitle("Retired Instructions")
      instructionsPlot<-instructionsPlot+theme(legend.position="none")
    
    
    ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-sv/", graph, "-time.pdf", sep=""), timePlot, width=8, height=5, units="in")
    ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-sv/", graph, "-branches.pdf", sep=""), branchesPlot, width=8, height=5, units="in")
    ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-sv/", graph, "-mispredictions.pdf", sep=""), mispredictionsPlot, width=8, height=5, units="in")
    ggsave(paste("../GraphsAreEasy/paper/results/",arch, "-sv/", graph, "-instructions.pdf", sep=""), instructionsPlot, width=8, height=5, units="in")
    
#    multiplot( timePlot, branchesPlot, mispredictionsPlot,instructionsPlot,cols=2)   

  }
}
