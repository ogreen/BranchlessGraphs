#======================================================================
# This script plots cumulative metrics, relative to the branch-based
# algorithm, versus iterations for all input graphs given a single
# algorithm and architecture.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("METRIC", "Time")
assign.if.undef ("SAVE.PDF", FALSE)

# Check parameters
stopifnot (METRIC %in% c ("Time", "Mispredictions", "Branches", "Instructions"))

#======================================================================
# Load and transform data
#======================================================================

Data.set <- load.xform.many (ALGS, ARCHS, GRAPHS)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

Totals <- merge (Data.set$Totals
                 , Summary[, c ("Comp", "Arch", "Graph", "Time.bry")]
                 , by=c ("Comp", "Arch", "Graph"))

setDevHD (l=15)
#Q <- qplot (Comp, Time.bry / Time.tot, data=Totals, facets=Arch ~ Graph, colour=Alg)
Q <- qplot (Comp, Speedup, data=Summary, facets=Arch ~ Graph, colour=Comp)
Q <- Q + geom_hline (yintercept=1, linetype="dashed", alpha=0.5)
Q <- set.hpcgarage.colours (Q)
print (Q)

if (SAVE.PDF) {
  outfilename <- "figs/summary.pdf"
  cat (sprintf ("Saving: %s ...\n", outfilename))
  setDevHD.pdf (outfilename, l=15)
  print (Q)
  dev.off ()
}

# eof
