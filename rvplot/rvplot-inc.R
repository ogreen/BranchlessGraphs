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
  
  perfdata <- read.table (paste ("../", arch, "-", alg, "/", graph, ".log", sep=""), sep="\t")
  names (perfdata) <- c ("Comp", "Alg", "Iters", "Time", "Mispreds", "Brs", "Insts", "Vs", "Es")
  perfdata$Comp <- with (perfdata, factor (Comp, levels=unique (Comp)))
  perfdata$Alg <- with (perfdata, factor (Alg, levels=unique (Alg)))
  
  return (perfdata)
}

# Reads a given subset of results file
load.perfdata.many <- function (Algs=NA, Archs=NA, Graphs=NA) {
  if (all (is.na (Algs))) { Algs <- ALGS; }
  if (all (is.na (Archs))) { Archs <- ARCHS; }
  if (all (is.na (Graphs))) { Graphs <- GRAPHS; }

  Data <- NULL
  for (alg in Algs) {
    for (arch in Archs) {
      for (graph in Graphs) {
        cat (sprintf ("Loading: (%s, %s, %s) ...\n", alg, arch, graph))
        Data.1 <- load.perfdata (alg, arch, graph)
        Data.1$Arch <- arch
        Data.1$Graph <- graph
        Data <- rbind (Data, Data.1)
      }
    }
  }

  # Convert fields to factors
  for (F in c ("Comp", "Alg", "Arch", "Graph")) {
    Data[, F] <- factor (Data[, F])
  }

  return (Data)
}

#======================================================================
# Load and transform data

load.xform.many <- function (Algs, Archs, Graphs) {
  # Raw data
  Data <- load.perfdata.many (Algs, Archs, Graphs)

  cat (sprintf ("Transforming...\n"))

  # Compute some totals for each (computation, algorithm, architecture, graph) combination
  Totals <- ddply (Data, .(Comp, Alg, Arch, Graph), summarise
                   , Time.tot=sum (Time)
                   , Iters.tot=max (Iters)+1  # Iters starts at 0, so add 1
                   , Mispreds.tot=sum (as.numeric (Mispreds))
                   , Brs.tot=sum (as.numeric (Brs))
                   , Insts.tot=sum (as.numeric (Insts))
                   )
  Data <- merge (Data, Totals, by=c ("Comp", "Alg", "Arch", "Graph"), sort=FALSE)

  # Extract summary data (total time & iterations) for the branch-based ("bry") version
  Branchy <- subset (Totals, Alg == "Branch-based")
  Branchy[, "Alg"] <- NULL # redundant; remove
  Branchy <- rename.col (Branchy, old="Time.tot", new="Time.bry")
  Branchy <- rename.col (Branchy, old="Iters.tot", new="Iters.bry")
  Branchy <- rename.col (Branchy, old="Mispreds.tot", new="Mispreds.bry")
  Branchy <- rename.col (Branchy, old="Brs.tot", new="Brs.bry")
  Branchy <- rename.col (Branchy, old="Insts.tot", new="Insts.bry")

  # Also extract summary branchless data
  Branchless <- subset (Totals, Alg == "Branch-avoiding")
  Branchless[, "Alg"] <- NULL # redundant; remove
  Branchless <- rename.col (Branchless, old="Time.tot", new="Time.brl")
  Branchless <- rename.col (Branchless, old="Iters.tot", new="Iters.brl")
  Branchless <- rename.col (Branchless, old="Mispreds.tot", new="Mispreds.brl")
  Branchless <- rename.col (Branchless, old="Brs.tot", new="Brs.brl")
  Branchless <- rename.col (Branchless, old="Insts.tot", new="Insts.brl")

  # Create a merged branchy vs. baseline summary
  Summary <- merge (Branchy, Branchless, by=c ("Comp", "Arch", "Graph"), sort=FALSE)
  Summary <- transform (Summary, Speedup=Time.bry / Time.brl)
  Summary <- transform (Summary, Mispreds.ratio=Mispreds.bry / Mispreds.brl)
  Summary <- transform (Summary, Brs.ratio=Brs.bry / Brs.brl)
  Summary <- transform (Summary, Insts.ratio=Insts.bry / Insts.brl)

  # Add branchy data as the "baseline"
  Data <- merge (Data, Summary, by=c ("Comp", "Arch", "Graph"), sort=FALSE)

  # Add cumulative attributes
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Time.cumul=cumsum (Time))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Mispreds.cumul=cumsum (as.numeric (Mispreds)))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Brs.cumul=cumsum (as.numeric (Brs)))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Insts.cumul=cumsum (as.numeric (Insts)))

  return (list (Data=Data, Totals=Totals, Branchy=Branchy, Branchless=Branchless, Summary=Summary))
}

#======================================================================
# Miscellaneous ggplot helper functions

# Generate an auto-scaled axis (linear or log2)
gen.axis.scale.auto <- function (Values, axis, scale="linear") {
  stopifnot (scale %in% c ("linear", "log2"))

  if (scale == "linear") {
    step <- gen.stepsize.auto (Values)$scaled
    Breaks <- gen_ticks_linear (Values, step)
    scale.func <- if (axis == "x") scale_x_continuous else scale_y_continuous
    return (scale.func (breaks=Breaks))
  }
  if (scale == "log2") {
    Breaks <- gen_ticks_log2 (Values)
    Labels <- genlabels.log2 (Breaks)
    scale.func <- if (axis == "x") scale_x_log2 else scale_y_log2
    return (scale.func (breaks=Breaks, labels=Labels))
  }
  
  return (NA)
}

# Add title with optional subtitle.
# Set 'func' to function to invoke, e.g., ggtitle, xlab, ...
# Adapted from: http://www.antoni.fr/blog/?p=39
add.title.optsub <- function (Q, func, main, sub=NA) {
  if (is.na (sub)) {
    Q <- Q + func (eval (parse (text=paste ("expression(atop(\"", main, "\",", "))", sep=""))))
  } else {
    Q <- Q + func (eval (parse (text=paste ("expression(atop(\"", main, "\",", " atop(\"", sub, "\",\"\")))", sep=""))))
  }
  return (Q)
}

#======================================================================
# eof
