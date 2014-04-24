source ("cmds-inc.R")
source ("util-inc.R")

#======================================================================
# Configuration parameters
#======================================================================

DATA.ROOT <- ".."

# List of algorithms
ALGS.ALL.MAP <- c ("sv"="SV", "bfs"="BFS/TD")
ALGS.ALL <- names (ALGS.ALL.MAP)
ALGS.ABBREV.MAP <- c ("SV"="sv", "BFS/TD"="bfs")

# List of available implementations
CODES.ALL.MAP <- c ("bb"="Branch-based", "bl"="Branch-avoiding")
CODES.ALL <- names (CODES.ALL.MAP)
CODES.ABBREV.MAP <- c ("Branch-based"="bb", "Branch-avoiding"="bl")

# List of available architectures
ARCHS.ALL.MAP <- c ("arn"="Cortex-A15", "hsw"="Haswell", "ivb"="Ivy Bridge", "bobcat"="Bobcat", "bonnell"="Bonnell", "pld"="Piledriver", "slv"="Silvermont")
ARCHS.ALL <- names (ARCHS.ALL.MAP)
ARCHS.ABBREV.MAP <- c ("Cortex-A15"="arn", "Haswell"="hsw", "Ivy Bridge"="ivb", "Bobcat"="bobcat", "Bonnell"="bonnell", "Piledriver"="pld", "Silvermont"="slv")

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
    arch.long <- ARCHS.ALL.MAP[arch]
    cat (sprintf ("... loading data for %s [%s] ...\n", arch.long, arch))
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
    Data.all[[arch.long]] <- Data.arch.all
  } # arch

  return (Data.all)
}
  
# Get a subset of the full data set for one architecture, as a data
# frame.  Note: arch, algs, and codes should be specified by long name
get.perfdf.arch <- function (Data, arch, algs, codes, verbose=TRUE) {
  stopifnot (is.list (Data))
  stopifnot ((length (arch) == 1) & (arch %in% as.vector (ARCHS.ALL.MAP)))
  stopifnot (all (algs %in% as.vector (ALGS.ALL.MAP)))
  stopifnot (all (codes %in% as.vector (CODES.ALL.MAP)))
  
  Df <- subset (Data[[arch]], Algorithm %in% algs & Implementation %in% codes)
  
  if (verbose) { cat ("\nFirst few rows of the relevant data:\n") }
  if (nrow (Df) > 0) { # matched
    Df$Time <- Df$Time * 1e9 # Convert to nanoseconds

    I.levels <- rev (levels (Df$Implementation)) # Put 'branch-based' before 'branch-avoiding'
    Df$Implementation <- with (Df, factor (Implementation, levels=I.levels))

    if (verbose) {
      print (head (Df, n=12))
      cat ("...\n\n")
    }
  } else if (verbose) {
    cat ("   (no data found)\n")
  }
  return (Df)
}

#======================================================================

# ARCHS: Long names, e.g., "Haswell", "Ivy Bridge", ...
# ALGS: Long names, e.g., "SV", "BFS/TD"
# CODES: Long names, e.g., "Branch-based", "Branch-avoiding"
get.file.suffix <- function (ARCHS, ALGS, CODES) {
  ARCHS.ABBREV <- sort (ARCHS.ABBREV.MAP[ARCHS])
  if (length (setdiff (ARCHS.ALL, ARCHS.ABBREV)) == 0) {
    archs.tag <- "all_archs"
  } else {
    archs.tag <- paste (ARCHS.ABBREV, collapse="_")
  }

  ALGS.ABBREV <- sort (ALGS.ABBREV.MAP[ALGS]) # convert to shorthand
  if (length (setdiff (ALGS.ALL, ALGS.ABBREV)) == 0) {
    algs.tag <- "all_algs"
  } else {
    algs.tag <- paste (ALGS.ABBREV, collapse="_")
  }

  CODES.ABBREV <- sort (CODES.ABBREV.MAP[CODES])
  if (length (setdiff (CODES.ALL, CODES.ABBREV)) == 0) {
    codes.tag <- "all_codes"
  } else {
    codes.tag <- paste (CODES.ABBREV, collapse="_")
  }
  
  outfile.suffix <- sprintf ("%s--%s--%s", archs.tag, algs.tag, codes.tag)
  return (outfile.suffix)
}

#======================================================================
# Functions for working with perfdata.frames, i.e., 'perfdfs'. 

# Given a perfdf, see what variables are available and classify
# them. Returns a categorized list, Vars. The categories are:
#
#   Vars$All -- All variables, i.e., all column names
#
#   Vars$Select.fit -- Variables to select a subset of data for
#       fitting, i.e., Algorithm, Implementation, and Graph
#
#   Vars$Index -- Variables to select a unique point (Index), i.e.,
#       Vars$Select.fit + Iteration
#
#   Vars$Data -- Data variables, corresponding mainly to the numerical
#       counter values.
#
#   Vars$Platform -- Variables specific to this platform
#   Vars$Load -- Variables related to load counters
#   Vars$Store -- Variables related to store counters
#
#   Vars$has.cycles -- TRUE if cycle counter ('Cycles') is available
#
get.perfdf.var.info <- function (Df, Data) {
  stopifnot (is.data.frame (Df))

  # Pull out lists of special variables
  Common.vars <- get.common.colnames (Data)
  All.vars <- get.all.colnames (Data)
  All.load.vars <- All.vars[grep ("^Loads.*", All.vars)]
  All.store.vars <- All.vars[grep ("^Stores.*", All.vars)]
  All.stall.vars <- All.vars[grep ("^Stalls.*", All.vars)]
  
  Df.vars <- colnames (Df)
  Select.fit.vars <- c ("Algorithm", "Implementation", "Graph")
  Index.vars <- c (Select.fit.vars, "Iteration") # aggregation vars
  Data.vars <- setdiff (Df.vars, Index.vars)
  Platform.vars <- setdiff (Df.vars, Common.vars)
  Load.vars <- intersect (Platform.vars, All.load.vars)
  Store.vars <- intersect (Platform.vars, All.store.vars)
  Stall.vars <- intersect (Platform.vars, All.stall.vars)
  has.cycles <- ("Cycles" %in% Platform.vars)

  Vars <- list ("All"=Df.vars
                , "Data"=Data.vars
                , "Platform"=Platform.vars
                , "Load"=Load.vars
                , "Store"=Store.vars
                , "Stall"=Stall.vars
                , "Select.fit"=Select.fit.vars
                , "Index"=Index.vars
                , "has.cycles"=has.cycles)
  return (Vars)
}

# Given a perfdf ('Df') and a desired normalizing variable ('by'),
# returns a new data frame with the corresponding per-iteration
# normalizing values. These values appear in a column of the new data
# frame named, 'Norm.vals'.
#
# When use.branch.based is TRUE, returns the normalizing values
# considering _only_ the branch-based implementation. This option is
# useful when, for example, comparing the instructions of different
# implementations with the branch-based implementation.
#
#
get.perfdf.norm <- function (Df, Vars, by, use.branch.based=FALSE) {
  # Compute normalization factors
  if (use.branch.based) {
    Merge.vars <- setdiff (Vars$Index, "Implementation")
    Df.norm <- subset (Df, Implementation == "Branch-based")
  } else {
    Merge.vars <- Vars$Index
    Df.norm <- Df
  }
  Df.norm <- Df.norm[, c (Merge.vars, by)]
  names (Df.norm)[names (Df.norm) == by] <- "Norm.vals"
  return (Df.norm)
}

# Given a perfdf ('Df'), normalizes its variables by the factor that
# appears in the given normalizer data frame ('Norm.df'). Returns a
# new normalized perfdf, with an additional column named, 'Norm.vals'.
#
# 'Norm.df' must have a particular form: it must contain a column
# named 'Norm.vals' (or, use 'by' to specify a different name). The
# rest of the columns of 'Norm.df' will be used as join variables when
# combining 'Norm.df' with 'Df'.
#
# Typically, the caller uses 'get.perfdf.norm' to produce 'Norm.df',
# so that input should satisfy the preconditions.
#
normalize.perfdf <- function (Df, Vars, Norm.df, by="Norm.vals") {
  stopifnot (is.data.frame (Df))
  stopifnot (is.data.frame (Norm.df))
  stopifnot (by %in% colnames (Norm.df))
  
  norm.var <- by
  join.vars <- setdiff (colnames (Norm.df), norm.var)
  Df.norm <- merge (Df, Norm.df, by=join.vars)

  func.norm <- function (X, N) colwise (function (X) X / N) (X)
  Normalizer <- as.vector (unlist (Df.norm[norm.var]))
  Df.norm[, Vars$Data] <- func.norm (Df.norm[, Vars$Data], Normalizer)
  return (Df.norm)
}

#======================================================================
# Aggregation on perfdf objects

# Apply a function 'reduce' to the numeric columns of Df
aggregate.perfdf <- function (Df, Vars, reduce) {
  stopifnot (is.data.frame (Df))
  stopifnot (is.list (Vars))
  stopifnot (is.function (reduce))

  f <- function (X) reduce (as.numeric (X))
  Df.reduce <- ddply (Df, Vars$Select.fit, colwise (f))
  return (Df.reduce)
}

# Compute aggregate totals
total.perfdf <- function (Df, Vars) {
  Df.tot <- aggregate.perfdf (Df, Vars, reduce=sum)

  # Iterations is an exception: compute max, not sum
  Df.max <- aggregate.perfdf (Df, Vars, reduce=max)
  Df.tot$Iteration <- Df.max$Iteration + 1
  return (Df.tot)
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
