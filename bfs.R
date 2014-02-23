rm(list = ls())
library(ggplot2)
source("multiplot.R")

perfdata = read.table("astro-ph.log", sep="\t")
names(perfdata) <- c("Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
perfdata$Algorithm <- factor(perfdata$Algorithm, levels=unique(perfdata$Algorithm))

timePlot <- 
  ggplot(perfdata, aes(x=Iteration, y=Time, colour=Algorithm)) +
  geom_line() +
  ylab("Time, ms") +
  ggtitle("Execution Time")

edgePlot <- 
  ggplot(perfdata, aes(x=Iteration, y=Edges)) +
  geom_line() +
  ylab("Traversed Edges") +
  ggtitle("Traversed Edges")

perfdata$TimePerEdge = perfdata$Time * 1.0e+3 / perfdata$Edges
timePerEdgePlot <- 
  ggplot(subset(perfdata, Iteration!=0), aes(x=Iteration, y=TimePerEdge, colour=Algorithm)) +
  geom_line() +
  ylab("Time per Edge, ms") +
  ggtitle("Execution Time")

perfdata$BranchesPerEdge = perfdata$Branches / perfdata$Edges
branchesPerEdgePlot <- 
  ggplot(subset(perfdata, Iteration!=0), aes(x=Iteration, y=BranchesPerEdge, colour=Algorithm)) +
  geom_line() +
  ylab("Branches per Edge") +
  ggtitle("Retired Branch Instructions")

perfdata$MispredictionsPerEdge = perfdata$Mispredictions / perfdata$Edges
mispredictionsPerEdgePlot <- 
  ggplot(subset(perfdata, Iteration!=0), aes(x=Iteration, y=MispredictionsPerEdge, colour=Algorithm)) +
  geom_line() +
  ylab("Branch Mispredictions per Edge") +
  ggtitle("Mispredicted Branch Instructions")

instructionsPerEdgePlot <- 
  ggplot(subset(perfdata, Iteration!=0), aes(x=Iteration, y=Instructions, colour=Algorithm)) +
  geom_line() +
  ylab("Instructions per Edge") +
  ggtitle("Retired Instructions")

multiplot(timePerEdgePlot, timePlot, edgePlot, branchesPerEdgePlot, mispredictionsPerEdgePlot, instructionsPerEdgePlot, cols=2)  