rm(list = ls())
library(ggplot2)

for (arch in c("ivb", "pld", "slv")) {
  for (graph in c("astro-ph", "auto", "ecology1", "preferentialAttachment")) {
    perfdata = read.table(paste(arch, "-sv/", graph, ".log", sep=""), sep="\t")
    names(perfdata) <- c("Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions")
    perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))
    
    timePlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Time, colour=Algorithm)) +
      geom_line() +
      ylab("Time, ms") +
      ggtitle("Execution Time")
    
    branchesPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Branches, colour=Algorithm)) +
      geom_line() +
      ylab("Branches") +
      ggtitle("Retired Branch Instructions")
    
    mispredictionsPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Mispredictions, colour=Algorithm)) +
      geom_line() +
      ylab("Branch Mispredictions") +
      ggtitle("Mispredicted Branch Instructions")
    
    instructionsPlot <- 
      ggplot(perfdata, aes(x=Iteration, y=Instructions, colour=Algorithm)) +
      geom_line() +
      ylab("Instructions") +
      ggtitle("Retired Instructions")
    
    ggsave(paste(arch, "-sv/", graph, "-time.png", sep=""), timePlot, width=8, height=5, units="in")
    ggsave(paste(arch, "-sv/", graph, "-branches.png", sep=""), branchesPlot, width=8, height=5, units="in")
    ggsave(paste(arch, "-sv/", graph, "-mispredictions.png", sep=""), mispredictionsPlot, width=8, height=5, units="in")
    ggsave(paste(arch, "-sv/", graph, "-instructions.png", sep=""), instructionsPlot, width=8, height=5, units="in")
  }
}
