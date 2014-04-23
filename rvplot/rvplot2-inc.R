source ("cmds-inc.R")
source ("util-inc.R")

#======================================================================
# Configuration parameters
#======================================================================

DATA.ROOT <- ".."

# List of algorithms
ALGS.ALL.MAP <- c ("sv"="SV", "bfs"="BFS/TD")
ALGS.ALL <- names (ALGS.ALL.MAP)

# List of available architectures
ARCHS.ALL.MAP <- c ("arn"="Cortex-A15", "hsw"="Haswell", "ivb"="Ivy Bridge", "bobcat"="Bobcat", "bonnell"="Bonnell", "pld"="Piledriver", "slv"="Silvermont")
ARCHS.ALL <- names (ARCHS.ALL.MAP)

# List of graph input problems
GRAPHS.ALL <- c("astro-ph", "audikw1", "auto", "coAuthorsDBLP", "coPapersDBLP", "cond-mat-2003", "cond-mat-2005", "ecology1", "ldoor", "power", "preferentialAttachment")
GRAPHS.CONCISE <- c("audikw1", "auto", "coAuthorsDBLP", "cond-mat-2005", "ldoor") # Reported in SC'14 submission

# Default header, for data files that don't have one
HEADERS.DEFAULT <- c ("Algorithm", "Implementation", "Iteration", "Time", "Mispredictions", "Branches", "Instructions", "Vertices", "Edges")
  
# ivb: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Loads.Retired	Stores.Retired	Stall.RS	Stall.SB	Stall.ROB	Branches	Mispredictions	Vertices	Edges
# hsw: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Loads.Retired	Stores.Retired	Stall.RS	Stall.SB	Stall.ROB	Branches	Mispredictions	Vertices	Edges
# pld: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Stall.SB	Stall.LB	Loads.Dispatched	Stores.Dispatched	Stall.LDQ	Branches	Mispredictions	Vertices	Edges
# slv: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Stall.ROB	Stall.RAT	Stall.MEC	Stall.AnyRS	Loads.RehabQ	Stores.RehabQ	Loads.Retired	Stores.Retired	Branches	Mispredictions	Vertices	Edges

#======================================================================
# Utility functions to read results data
#======================================================================

# Load data for one (algorithm, architecture, graph) instance
load.perfdata.one <- function (alg, arch, graph, fatal=FALSE) {
  stopifnot (alg %in% ALGS.ALL)
  stopifnot (arch %in% ARCHS.ALL)
  stopifnot (graph %in% GRAPHS.ALL)

  filename <- sprintf ("%s/%s-%s/%s.log", DATA.ROOT, arch, alg, graph) # e.g., "../hsw-sv/audikw1.log"
  check.cond (file.exists (filename), msg=sprintf ("*** Data file, '%s', does not exist ***", filename), fatal=fatal)

  # First, try to read with a header
  Data <- read.table (filename, header=TRUE, sep="\t")
  if (names (Data)[1] != "Algorithm") { # file has no header
    Data <- read.table (filename, header=FALSE, sep="\t")
    colnames (Data) <- HEADERS.DEFAULT
  }

  # HACK: Correct a header typo, e.g., in "hsw" data
  Data <- rename.col (Data, old="Itetarion", new="Iteration")

  # HACK: In some files, 'Time' is seconds; in others, 'Time' is
  # nanoseconds. Convert all to seconds.
  if ("Cycles" %in% colnames (Data)) {
    Data$Time <- Data$Time * 1e-9
  }

  # Add the input graph as a field
  Data$Graph <- graph

  # Reorder fields so that (Algorithm, Implementation, Graph) appear first
  Cols <- colnames (Data)
  Cols.first <- c ("Algorithm", "Implementation", "Graph", "Iteration", "Vertices", "Edges", "Time")
  Cols.remaining <- setdiff (Cols, Cols.first)
  Data <- Data[, c (Cols.first, Cols.remaining)]

  return (Data)
}

# Load many data sets. Returns a list parameterized by
# architecture. Set Algs, Archs, and/or Graphs to 'NA' to load all
# valid possibilities.
load.perfdata.many <- function (Algs=NULL, Archs=NULL, Graphs=NULL, fatal=FALSE) {
  if (is.null (Algs)) { Algs <- ALGS.ALL }
  if (is.null (Archs)) { Archs <- ARCHS.ALL }
  if (is.null (Graphs)) { Graphs <- GRAPHS.ALL }

  # Validate input parameters
  stopifnot (all (Algs %in% ALGS.ALL))
  stopifnot (all (Archs %in% ARCHS.ALL))
  stopifnot (all (Graphs %in% GRAPHS.ALL))

  Data.all <- list ()
  for (arch in Archs) {
    cat (sprintf ("... loading data for %s [%s] ...\n", ARCHS.ALL.MAP[arch], arch))
    Data.arch.all <- NULL
    for (alg in Algs) {
      for (graph in Graphs) {
        Data.arch.one <- load.perfdata.one (alg, arch, graph, fatal=fatal)
        if (length (Data.arch.all) == 0) {
          Data.arch.all <- Data.arch.one
        } else {
          headers.match <- all (names (Data.arch.one) == names (Data.arch.all))
          if (check.cond (headers.match, sprintf ("*** Headers mismatch! arch=%s, alg=%s, graph=%s ***", arch, alg, graph), fatal)) {
            Data.arch.all <- rbind (Data.arch.all, Data.arch.one)
          }
        }
      } # graph
    } # alg
    Data.all[[arch]] <- Data.arch.all
  } # arch

  return (Data.all)
}
  
#======================================================================
# Plotting utilities

# 'base' has units of points
set.all.font.sizes <- function (Q, base=10) {
  Q <- Q + theme (axis.text.x=element_text (size=base))
  Q <- Q + theme (legend.text=element_text (size=1.4*base))
  Q <- Q + theme (axis.text.y=element_text (size=base))
  Q <- Q + theme (plot.title=element_text (size=2*base))
  Q <- Q + theme (strip.text.x = element_text (size=1.2*base))
  return (Q)
}
#======================================================================
# eof
