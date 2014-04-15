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

  # Convert fields to factors
  Data$Algorithm <- factor (Data$Algorithm)
  Data$Arch <- factor (Data$Arch)
  Data$Graph <- factor (Data$Graph)

  return (Data)
}

#======================================================================
# Load and transform data
#======================================================================
Algs <- "sv"
Archs <- "arn"
Graphs <- NA

# Raw data
Data <- load.perfdata.all (Algs, Archs, Graphs)

# Compute some totals for each (computation, algorithm, architecture, graph) combination
Totals <- ddply (Data, .(Comp, Algorithm, Arch, Graph), summarise
                 , Time.total=sum (Time)
                 , Iters.total=max (Iteration)
                 , Mispredictions.total=sum (as.numeric (Mispredictions))
                 , Branches.total=sum (as.numeric (Branches))
                 , Instructions.total=sum (as.numeric (Instructions))
                 )
Data <- merge (Data, Totals, by=c ("Comp", "Algorithm", "Arch", "Graph"), sort=FALSE)

# Extract summary data (total time & iterations) for the branch-based ("branchy") version
Branchy <- subset (Totals, Algorithm == "Branch-based")
Branchy[, "Algorithm"] <- NULL # redundant; remove
Branchy <- rename.col (Branchy, old="Time.total", new="Time.branchy")
Branchy <- rename.col (Branchy, old="Iters.total", new="Iters.branchy")

# Also extract summary branchless data
Branchless <- subset (Totals, Algorithm == "Branch-avoiding")
Branchless[, "Algorithm"] <- NULL # redundant; remove
Branchless <- rename.col (Branchless, old="Time.total", new="Time.branchless")
Branchless <- rename.col (Branchless, old="Iters.total", new="Iters.branchless")

# Create a merged branchy vs. baseline summary
Summary <- merge (Branchy, Branchless, by=c ("Comp", "Arch", "Graph"), sort=FALSE)
Summary <- transform (Summary, Speedup=Time.branchy / Time.branchless)

# Add branchy data as the "baseline"
Data <- merge (Data, Summary, by=c ("Comp", "Arch", "Graph"), sort=FALSE)

# Add cumulative attributes
Data <- ddply (Data, .(Comp, Algorithm, Arch, Graph), transform, Time.cumul=cumsum (Time))
Data <- ddply (Data, .(Comp, Algorithm, Arch, Graph), transform, Mispredictions.cumul=cumsum (as.numeric (Mispredictions)))
Data <- ddply (Data, .(Comp, Algorithm, Arch, Graph), transform, Branches.cumul=cumsum (as.numeric (Branches)))
Data <- ddply (Data, .(Comp, Algorithm, Arch, Graph), transform, Instructions.cumul=cumsum (as.numeric (Instructions)))

#======================================================================
# Plot
#======================================================================

# Copy of data for plotting purposes
Data.plot <- Data

# Choose x-variable
Data.plot <- transform (Data.plot, X=Iteration / Iters.branchy)
x.label <- "Iterations (normalized by branch-based algorithm)"

# Choose y-variable
Data.plot <- transform (Data.plot, Y=Time.cumul / Time.branchy)
y.label <- "Cumulative time, relative to branch-based algorithm"
y.step <- gen.stepsize.auto (Data.plot$Y)$base
Y.breaks <- gen_ticks_linear (Data.plot$Y, y.step)
y.scale.func <- scale_y_continuous (breaks=Y.breaks)

# Apply any subplot reordering
Order.by.Speedup <- rev (order (Summary$Speedup))
Data.plot$Graph <- with (Data.plot, factor (Graph, levels=levels (Graph)[Order.by.Speedup]))

# Generate plot
Q <- ggplot (Data.plot, aes (x=X, y=Y))
Q <- Q + geom_point (aes (colour=Algorithm))
Q <- Q + geom_line (aes (colour=Algorithm))
Q <- Q + xlab (x.label)
Q <- Q + y.scale.func + ylab ("")
Q <- Q + geom_hline (aes (yintercept=1), linetype="dashed")
Q <- Q + facet_wrap (~ Graph)
Q <- Q + ggtitle (y.label)
Q <- Q + theme (legend.position="bottom")
Q <- set.hpcgarage.colours (Q)

#setDevPage.portrait ()
setDevSlide ()
print (Q)

# eof
