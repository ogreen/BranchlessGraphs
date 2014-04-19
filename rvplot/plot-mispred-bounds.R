#======================================================================
# This script creates a plot that summarizes how closely the branch
# avoiding algorithms attain the estimated lower bounds on branch
# mispredictions.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("COMP", "sv") # "sv" or "bfs"
assign.if.undef ("ARCHS", NA)
assign.if.undef ("GRAPHS", NA)
assign.if.undef ("SAVE.PDF", FALSE)

# Check parameters
stopifnot (COMP %in% c ("sv", "bfs"))

#======================================================================
# Load and transform data
#======================================================================

comp.str <- if (COMP == "sv") "SV" else "BFS/TD"

Data.set <- load.xform.many (COMP, ARCHS, GRAPHS)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

Branchy <- with (Summary, data.frame (V.hat=Vs.tot.bry
                                      , M=Mispreds.tot.bry
                                      , Alg="Branch-based"
                                      , Arch=Arch
                                      , Graph=Graph))
Branchless <- with (Summary, data.frame (V.hat=Vs.tot.brl
                                         , M=Mispreds.tot.brl
                                         , Alg="Branch-avoiding"
                                         , Arch=Arch
                                         , Graph=Graph))

All <- rbind (Branchy, Branchless)
All <- transform (All, Alg.tag=gsub ("Branch-", "Branch-\n", Alg))
All$Alg.tag <- with (All, factor (Alg.tag, levels=rev (levels (Alg.tag))))

All$Arch <- with (All, mapvalues (Arch, from=ARCHS.ALL, to=ARCHS.NAMES))

#======================================================================
# Plot(s)
#======================================================================

cat (sprintf ("Plotting...\n"))

if (COMP == "sv") {
  alg.display.tag <- "Shiloach-Vishkin Connected Components"
} else {
  alg.display.tag <- "Top-down Breadth-First Search"
}

Q <- qplot (Alg.tag, M/V.hat, data=All, geom="bar", stat="identity"
            , fill=Alg.tag, shape=Graph, facets=Arch ~ Graph)
Q <- Q + geom_hline (yintercept=1, colour="black", linetype="dashed") # add y=1 reference line
Q <- Q + geom_hline (yintercept=3, colour="black", linetype="dashed") # add y=3 reference line
Q <- Q + xlab ("")
Q <- Q + ylab ("")
Q <- add.title.optsub (Q, ggtitle
                       , main=sprintf ("%s Branch Mispredictions", alg.display.tag)
                       , sub="(relative to lower-bound, at y=1)")
#Q <- Q + theme (axis.title.x = element_blank())
Q <- Q + theme (legend.position="none") # hide legend
Q <- set.hpcgarage.colours (Q)
Q <- set.hpcgarage.fill (Q)

setDevPage.portrait ()
print (Q)
                           

# Print
outfilename <- sprintf ("figs/mispred-bounds--%s.pdf", COMP)
cat (sprintf ("Output file: %s\n", outfilename))
if (!SAVE.PDF) {
  cat (sprintf ("--> Not saving.\n"))
} else {
  cat (sprintf ("--> Printing...\n"))
  setDevPage.portrait.pdf (outfilename, l=10)
  print (Q)
  dev.off ()
}

# eof
