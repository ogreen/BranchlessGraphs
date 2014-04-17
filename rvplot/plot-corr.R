#======================================================================
# This script plots per-iteration metrics, relative to the
# branch-based algorithm for all input graphs given a single algorithm
# and architecture.
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

comp.str <- if (COMP == "sv") "SV" else "BFS/TD"

#======================================================================
# Load and transform data
#======================================================================

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
  # Things I tried that didn't look informative
  ggpairs (subset (D, Comp == "SV" & Arch=="hsw" & Alg == "Branch-based")[, c ("T", "I", "B", "M")])
  ggpairs (subset (D, Comp == "SV" & Alg == "Branch-based"), columns=c ("T", "I", "B", "M"), colour="Graph", shape="Arch")
  ggpairs (subset (D, Comp == "SV" & Alg == "Branch-based")[, c ("T", "I", "B", "M")], colour=Arch)
}

setDevSquare ()
Q <- ggpairs (D, columns=c ("T", "I", "B", "M"), colour="Arch"
              , upper=list (continuous="points", combo="dot")
              , lower=list (continuous="cor")
              )
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
