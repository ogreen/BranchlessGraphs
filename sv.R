rm(list = ls())
library(ggplot2)
source("multiplot.R", local=TRUE)

perfdata = read.table("slv-sv/astro-ph.log", sep="\t")
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

multiplot(timePlot, instructionsPlot, branchesPlot, mispredictionsPlot, cols=2)  