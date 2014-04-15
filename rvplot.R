source ("cmds-inc.R")
source ("util-inc.R")

#======================================================================
# "Space" of possible results
#======================================================================
ALGS <- c ("sv", "bfs")
ARCHS <- c ("arn","hsw","bobcat", "bonnell","pld","slv")
GRAPHS <- c("astro-ph", "audikw1", "auto", "coAuthorsDBLP", "coPapersDBLP", "cond-mat-2003", "cond-mat-2005", "ecology1", "ldoor", "power", "preferentialAttachment")

#======================================================================
# Utility functions to read results data
#======================================================================

# Reads one results file
load.perfdata <- function (alg, arch, graph) {
  stopifnot (alg %in% ALGS)
  stopifnot (arch %in% ARCHS)
  stopifnot (graph %in% GRAPHS)
  
  perfdata <- read.table (paste (arch, "-", alg, "/", graph, ".log", sep=""), sep="\t")
  if (alg == "sv") {
    names (perfdata) <- c ("Comp", "Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
    perfdata$Algorithm <- factor (perfdata$Algorithm, levels=unique (perfdata$Algorithm))
  } else {
    stopifnot (alg == "bfs")
    names (perfdata) <- c ("Comp", "Algorithm", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
    perfdata$Algorithm <- factor (perfdata$Algorithm, levels=unique (perfdata$Algorithm))
  }
  return (perfdata)
}

# Reads a given subset of results file
load.perfdata.all <- function (Algs=NA, Archs=NA, Graphs=NA) {
  if (all (is.na (Algs))) { Algs <- ALGS; }
  if (all (is.na (Archs))) { Archs <- ARCHS; }
  if (all (is.na (Graphs))) { Graphs <- GRAPHS; }

  Data <- NULL
  for (alg in Algs) {
    for (arch in Archs) {
      for (graph in Graphs) {
        Data.1 <- load.perfdata (alg, arch, graph)
        Data.1$Arch <- arch
        Data.1$Graph <- graph
        Data <- rbind (Data, Data.1)
      }
    }
  }
  return (Data)
}

#======================================================================
# Load and transform data
#======================================================================
Algs <- "bfs"
Archs <- "arn"
Graphs <- NA

# Raw data
Data <- load.perfdata.all (Algs, Archs, Graphs)

# Compute some totals for each (computation, algorithm, architecture, graph) combination
Totals <- ddply (Data, .(Comp, Algorithm, Arch, Graph), summarise
                 , Time.total=sum (Time)
                 , Iters.total=max (Iteration)
                 )
Data <- merge (Data, Totals, by=c ("Comp", "Algorithm", "Arch", "Graph"))

# Baseline is branch-based
Branchy.time <- rename.col (subset (Total.time, Algorithm == "Branch-based"), old="Time.total", new="Time.0")
Data <- merge (Data, Branchy.time, by=c ("Comp", "Arch", "Graph"))
Branchy.iters <- rename.col (subset (Total.iters, Algorithm == "Branch-based"), old="Iters.total", new="Iters.0")
Data <- merge (Data, Branchy.iters, by=c ("Comp", "Arch", "Graph"))

# Add column with the cumulative time over iterations for each algorithm
Data <- ddply (Data, .(Comp, Algorithm, Arch, Graph), transform, Time.cumul=cumsum (Time))

#======================================================================
# Plot
#======================================================================

Q <- qplot (Iteration/Iters.0, Time.cumul/Time.0, data=Data, geom=c ("point", "line"), colour=Algorithm) + facet_wrap (~ Graph)

setDevHD (l=15)
print (Q)

# eof
