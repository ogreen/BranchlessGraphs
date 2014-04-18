#======================================================================
# This script creates a plot that summarizes the correlations among
# Time, Mispredictions, Branches, and Instructions.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("COMP", "sv")
assign.if.undef ("ALG", "Branch-based")
assign.if.undef ("ARCHS", NA)
assign.if.undef ("GRAPHS", NA)
assign.if.undef ("SAVE.PDF", FALSE)

# Check parameters
stopifnot (COMP %in% c ("sv", "bfs"))
stopifnot (ALG %in% c ("Branch-based", "Branch-avoiding"))

#======================================================================
# Load and transform data
#======================================================================

comp.str <- if (COMP == "sv") "SV" else "BFS/TD"

Data.set <- load.xform.many (COMP, ARCHS, GRAPHS)
Data <- Data.set[["Data"]]

cat (sprintf ("Analyzing...\n"))

Cols <- c ("Comp", "Arch", "Graph", "Alg", "Iters", "Time", "Mispreds", "Brs", "Insts", "Vs", "Es")
D <- transform (subset (Data, Comp == comp.str & Alg == ALG)[, Cols])
D <- transform (D, T=Time/Es*1e9, I=Insts/Es, B=Brs/Es, M=Mispreds/Es)

# Correlations
Rho <- ddply (D, .(Comp, Arch, Alg), summarise
              , TI=cor (T, I)
              , TB=cor (T, B)
              , TM=cor (T, M)
              )

#======================================================================
# Plot(s)
#======================================================================

library (GGally)

cat (sprintf ("Plotting...\n"))

# Make correlation matrix plots
if (FALSE) {
  # [rv] Things I tried that didn't look informative
  ggpairs (subset (D, Comp == "SV" & Arch=="hsw" & Alg == "Branch-based")[, c ("T", "I", "B", "M")])
  ggpairs (subset (D, Comp == "SV" & Alg == "Branch-based"), columns=c ("T", "I", "B", "M"), colour="Graph", shape="Arch")
  ggpairs (subset (D, Comp == "SV" & Alg == "Branch-based")[, c ("T", "I", "B", "M")], colour=Arch)
}

setDevSquare ()
# Doesn't work: ggplot <- function (...) set.hpcgarage.colours (ggplot2::ggplot(...))
Q <- ggpairs (D, columns=c ("T", "I", "B", "M"), colour="Arch"
              , upper=list (continuous="points", combo="dot")
              , lower=list (continuous="cor")
              )

# HACK: to fix colour palette
# See also: http://stackoverflow.com/questions/22237783/user-defined-colour-palette-in-r-and-ggpairs
for (i in seq (1, 4)) {
  for (j in seq (1, 4)) {
    P <- getPlot (Q, i, j)
    P.new <- set.hpcgarage.colours (P)
    Q <- putPlot (Q, P.new, i, j)
  }
}

print (Q)

outfilename <- sprintf ("figs/corr--%s--%s.pdf", COMP, ALG)
cat (sprintf ("Output file: %s\n", outfilename))
if (!SAVE.PDF) {
  cat (sprintf ("--> Not saving.\n"))
} else {
  cat (sprintf ("--> Printing...\n"))
  setDevSquare.pdf (outfilename, l=10)
  print (Q)
  dev.off ()
}

#======================================================================
# eof
