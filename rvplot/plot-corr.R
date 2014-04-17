#======================================================================
# This script plots per-iteration metrics, relative to the
# branch-based algorithm for all input graphs given a single algorithm
# and architecture.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("COMPUTATIONS", NA)
assign.if.undef ("ARCHITECTURES", NA)
assign.if.undef ("METRIC", "Time")
assign.if.undef ("CUMULATIVE", FALSE)
assign.if.undef ("SCALE", "xy")
assign.if.undef ("SHOW.POINTS", TRUE)
assign.if.undef ("SHOW.LINES", TRUE)
assign.if.undef ("SAVE.PDF", FALSE)

# Check parameters
if (all (is.na (ARCHITECTURES))) { # by default, plot all architectures
  ARCHITECTURES <- ARCHS
}
stopifnot (METRIC %in% c ("Time", "Mispredictions", "Branches", "Instructions"))
stopifnot (SCALE %in% c ("xy", "logx", "logy", "loglog"))

#======================================================================
# Load and transform data
#======================================================================

Data.set <- load.xform.many (COMPUTATIONS, ARCHITECTURES, GRAPHS)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

S <- transform (Summary, Norm=Iters.tot.bry * Es.bry)
S <- transform (S
                , T.bry=Time.tot.bry / Norm * 1e9
                , I.bry=Insts.tot.bry / Norm
                , M.bry=Mispreds.tot.bry / Norm
                , B.bry=Brs.tot.bry / Norm)

D <- transform (Data[, c ("Comp", "Arch", "Graph", "Alg"
                          , "Iters", "Time", "Mispreds", "Brs", "Insts"
                          , "Vs.x", "Es.x")]
                , Vs=Vs.x, Es=Es.x)
D$Vs.x <- NULL
D$Es.x <- NULL
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

cat (sprintf ("Plotting...\n"))
stopifnot (FALSE)

# Make a correlation matrix plots
ggpairs (data=S, columns=c ("T.bry", "I.bry", "B.bry", "M.bry"))
ggpairs (subset (D, Arch=="hsw" & Alg == "Branch-based")[, c ("T", "I", "B", "M")])

#======================================================================
# eof
