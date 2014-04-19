source ("cmds-inc.R")
source ("util-inc.R")

#======================================================================
# "Space" of possible results
#======================================================================
COMPS.ALL <- c ("sv", "bfs")
ARCHS.ALL <- c ("arn","hsw","ivb","bobcat", "bonnell", "pld", "slv")
#GRAPHS.ALL <- c("astro-ph", "audikw1", "auto", "coAuthorsDBLP", "coPapersDBLP", "cond-mat-2003", "cond-mat-2005", "ecology1", "ldoor", "power", "preferentialAttachment")
GRAPHS.ALL <- c("audikw1", "auto", "coAuthorsDBLP", "cond-mat-2005", "ldoor")

ARCHS.X <- c ("hsw", "ivb", "slv", "pld") # Architectures with extended counters
HEADERS <- c ("Comp", "Alg", "Iters", "Time", "Mispreds", "Brs", "Insts", "Vs", "Es")
HEADERS.X <- c ("Comp", "Alg", "Iters", "Time", "Cycles", "Insts", "Loads", "Stores", "Brs", "Mispreds", "Vs", "Es")

# ivb: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Loads.Retired	Stores.Retired	Stall.RS	Stall.SB	Stall.ROB	Branches	Mispredictions	Vertices	Edges
# hsw: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Loads.Retired	Stores.Retired	Stall.RS	Stall.SB	Stall.ROB	Branches	Mispredictions	Vertices	Edges
# pld: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Stall.SB	Stall.LB	Loads.Dispatched	Stores.Dispatched	Stall.LDQ	Branches	Mispredictions	Vertices	Edges
# slv: Algorithm	Implementation	Iteration	Time	Cycles	Instructions	Stall.ROB	Stall.RAT	Stall.MEC	Stall.AnyRS	Loads.RehabQ	Stores.RehabQ	Loads.Retired	Stores.Retired	Branches	Mispredictions	Vertices	Edges

stopifnot (all (HEADERS %in% HEADERS.X))

#======================================================================
# Utility functions to read results data
#======================================================================

# Reads one results file
load.perfdata <- function (comp, arch, graph, disable.x=FALSE) {
  stopifnot (comp %in% COMPS.ALL)
  stopifnot (arch %in% ARCHS.ALL)
  stopifnot (graph %in% GRAPHS.ALL)

  is.x <- arch %in% ARCHS.X
  
  perfdata <- read.table (paste ("../", arch, "-", comp, "/", graph, ".log", sep=""), sep="\t", header=is.x)
  if (!is.x) {
    names (perfdata) <- HEADERS
  } else {
    perfdata <- rename (perfdata, c (NULL
                                     , "Algorithm"="Comp"
                                     , "Implementation"="Alg"
                                     , "Iteration"="Iters"
                                     , "Instructions"="Insts"
                                     , "Loads.Retired"="Loads"
                                     , "Stores.Retired"="Stores"
                                     , "Stall.RS"="Stalls.rs"
                                     , "Stall.SB"="Stalls.sb"
                                     , "Stall.ROB"="Stalls.rob"
                                     , "Branches"="Brs"
                                     , "Mispredictions"="Mispreds"
                                     , "Vertices"="Vs"
                                     , "Edges"="Es"
                                     , "Stall.LB"="Stalls.lb"
                                     , "Loads.Dispatched"="Loads"
                                     , "Stores.Dispatched"="Stores"
                                     , "Stall.LDQ"="Stalls.ldq"
                                     , "Stall.RAT"="Stalls.rat"
                                     , "Stall.MEC"="Stalls.mec"
                                     , "Stall.AnyRS"="Stalls.anyrs"
                                     , "Loads.RehabQ"="Loads.rehabq"
                                     , "Stores.RehabQ"="Stores.rehabq"
                                     , "Itetarion"="Iters"
                                     ))
  }
  if (is.x) {
    if (!all (HEADERS.X %in% colnames (perfdata))) {
      cat ("*** ERROR ***\n")
      cat ("HEADERS.X == ", HEADERS.X, "\n")
      cat ("colnames (perfdata) == ", colnames (perfdata))
      cat ("*** END ERROR ***\n")
    }
    perfdata <- perfdata[, if (disable.x) HEADERS else HEADERS.X]
  }

  perfdata$Comp <- with (perfdata, factor (Comp, levels=unique (Comp)))
  perfdata$Alg <- with (perfdata, factor (Alg, levels=unique (Alg)))
  
  return (perfdata)
}

# Reads a given subset of results file
load.perfdata.many <- function (Comps=NA, Archs=NA, Graphs=NA) {
  if (all (is.na (Comps))) { Comps <- COMPS.ALL; }
  if (all (is.na (Archs))) { Archs <- ARCHS.ALL; }
  if (all (is.na (Graphs))) { Graphs <- GRAPHS.ALL; }

  is.x <- all (Archs %in% ARCHS.X) # Only allow extended data frame if available on all platforms

  Data <- NULL
  for (comp in Comps) {
    for (arch in Archs) {
      for (graph in Graphs) {
        cat (sprintf ("Loading: (%s, %s, %s) ...\n", comp, arch, graph))
        Data.1 <- load.perfdata (comp, arch, graph, disable.x=!is.x)
        Data.1$Arch <- arch
        Data.1$Graph <- graph
        if (!all (colnames (Data.1) %in% colnames (Data))) {
          cat ("*** Uh oh: columns mismatch! ***\n")
          cat (colnames (Data)) ; cat ("\n")
          cat (colnames (Data.1)) ; cat ("\n*** END ERROR ***\n")
        }
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

  # Compute per-edge metrics
  Data <- transform (Data, Time.E=Time / Es)
  Data <- transform (Data, Mispreds.E=Mispreds / Es)
  Data <- transform (Data, Brs.E=Brs / Es)
  Data <- transform (Data, Insts.E=Insts / Es)

  cat (sprintf ("Transforming...\n"))

  all.is.x <- all (Archs %in% ARCHS.X)

  # Compute some useful aggregates for each (computation, algorithm, architecture, graph) combination
  Aggs <- ddply (Data, .(Comp, Alg, Arch, Graph), summarise
                 , Time.min=min (as.numeric (Time))
                 , Time.tot=sum (as.numeric (Time))
                 , Iters.tot=max (Iters)+1  # Iters starts at 0, so add 1
                 , Mispreds.min=min (Mispreds)
                 , Mispreds.tot=sum (as.numeric (Mispreds))
                 , Brs.min=min (Brs)
                 , Brs.tot=sum (as.numeric (Brs))
                 , Insts.min=min (Insts)
                 , Insts.tot=sum (as.numeric (Insts))
                 , Vs.tot=sum (as.numeric (Vs))
                 , Es.tot=sum (as.numeric (Es))
                 )
  if (all.is.x) {
    Aggs <- ddply (Data, .(Comp, Alg, Arch, Graph), summarise
                   , Time.min=min (Time)
                   , Time.tot=sum (as.numeric (Time))
                   , Cycles.min=min (Cycles)
                   , Cycles.tot=sum (as.numeric (Cycles))
                   , Iters.tot=max (Iters)+1  # Iters starts at 0, so add 1
                   , Mispreds.min=min (Mispreds)
                   , Mispreds.tot=sum (as.numeric (Mispreds))
                   , Brs.min=min (Brs)
                   , Brs.tot=sum (as.numeric (Brs))
                   , Insts.min=min (Insts)
                   , Insts.tot=sum (as.numeric (Insts))
                   , Loads.min=min (Loads), Loads.tot=sum (as.numeric (Loads))
                   , Stores.min=min (Stores), Stores.tot=sum (as.numeric (Stores))
#                   , Stalls.rs.min=min (Stalls.rs), Stalls.rs.tot=sum (as.numeric (Stalls.rs))
#                   , Stalls.sb.min=min (Stalls.sb), Stalls.sb.tot=sum (as.numeric (Stalls.sb))
#                   , Stalls.rob.min=min (Stalls.rob), Stalls.rob.tot=sum (as.numeric (Stalls.rob))
                   )
  }
  Data <- merge (Data, Aggs, by=c ("Comp", "Alg", "Arch", "Graph"), sort=FALSE)

  # Extract summary data (total time & iterations) for the branch-based ("bry") version
  Branchy <- subset (Aggs, Alg == "Branch-based")
  Branchy[, "Alg"] <- NULL # redundant; remove

  # Also extract summary branchless data
  Branchless <- subset (Aggs, Alg == "Branch-avoiding")
  Branchless[, "Alg"] <- NULL # redundant; remove

  # Create a merged branchy vs. baseline summary
  Summary <- merge (Branchy, Branchless, by=c ("Comp", "Arch", "Graph")
                    , suffixes=c (".bry", ".brl")
                    , sort=FALSE)
  Summary <- transform (Summary, Speedup=Time.tot.bry / Time.tot.brl)
  Summary <- transform (Summary, Mispreds.ratio=Mispreds.tot.bry / Mispreds.tot.brl)
  Summary <- transform (Summary, Brs.ratio=Brs.tot.bry / Brs.tot.brl)
  Summary <- transform (Summary, Insts.ratio=Insts.tot.bry / Insts.tot.brl)

  # Add branchy data as the "baseline"
  Data <- merge (Data, Summary, by=c ("Comp", "Arch", "Graph"), sort=FALSE)

  # Add cumulative attributes
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Time.cumul=cumsum (as.numeric (Time)))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Mispreds.cumul=cumsum (as.numeric (Mispreds)))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Brs.cumul=cumsum (as.numeric (Brs)))
  Data <- ddply (Data, .(Comp, Alg, Arch, Graph), transform, Insts.cumul=cumsum (as.numeric (Insts)))

  return (list (Data=Data, Aggs=Aggs, Branchy=Branchy, Branchless=Branchless, Summary=Summary))
}

#======================================================================
# eof
