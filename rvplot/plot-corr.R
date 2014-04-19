#======================================================================
# This script creates a plot that summarizes the correlations among
# Time, Mispredictions, Branches, and Instructions.
#
# Example:
# rm (list=ls ()) ; COMP <- "bfs" ; ALG <- "Branch-based" ; ARCHS <- "hsw" ; source ("plot-corr.R")
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

is.x <- all (levels (Data$Arch) %in% ARCHS.X)

Cols <- c ("Comp", "Arch", "Graph", "Alg", "Iters"
           , "Time", "Mispreds", "Brs", "Insts"
           , "Vs", "Es")
if (is.x) {
  Cols <- c (Cols
             , "Cycles", "Loads", "Stores"
#           , "Stalls.rs", "Stalls.sb", "Stalls.rob")
             )
}
D <- transform (subset (Data, Comp == comp.str & Alg == ALG)[, Cols])
if (is.x) {
  D <- transform (D, T=Cycles/Es)
} else {
  D <- transform (D, T=Time/Es*ifelse (Arch %in% ARCHS.X, 1, 1e9))
}
D <- transform (D, I=Insts/Es, B=Brs/Es, M=Mispreds/Es)
if (is.x) {
  D <- transform (D, L=Loads/Es, S=Stores/Es)
#  D <- transform (D, St.rs=Stalls.rs/Es, St.sb=Stalls.sb/Es, St.rob=Stalls.rob/Es)
}

# Correlations
if (is.x) {
  Rho <- ddply (D, .(Comp, Arch, Alg), summarise
                , TI=cor (T, I)
                , TB=cor (T, B)
                , TM=cor (T, M)
                , TL=cor (T, L)
                , TS=cor (T, S)
#                , TSt.rs=cor (T, St.rs)
#                , TSt.sb=cor (T, St.sb)
#                , TSt.rob=cor (T, St.rob)
                )
} else {
  Rho <- ddply (D, .(Comp, Arch, Alg), summarise
                , TI=cor (T, I)
                , TB=cor (T, B)
                , TM=cor (T, M)
                )
}

#======================================================================
# Make correlation matrix plot(s)
#======================================================================

library (GGally)

cat (sprintf ("Plotting...\n"))

Corr.cols <- c ("T", "I", "B", "M")
if (is.x) {
  Corr.cols <- c (Corr.cols, "L", "S") #, "St.rs")
}

setDevSquare ()
# Doesn't work: ggplot <- function (...) set.hpcgarage.colours (ggplot2::ggplot(...))
Q <- ggpairs (D, columns=Corr.cols, colour="Arch"
              , upper=list (continuous="points", combo="dot")
              , lower=list (continuous="cor")
              )

# HACK: to fix colour palette
# See also: http://stackoverflow.com/questions/22237783/user-defined-colour-palette-in-r-and-ggpairs
for (i in seq (1, length (Corr.cols))) {
  for (j in seq (1, length (Corr.cols))) {
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
