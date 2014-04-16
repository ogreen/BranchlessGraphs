#======================================================================
# This script plots cumulative metrics, relative to the branch-based
# algorithm, versus iterations for all input graphs given a single
# algorithm and architecture.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("ALG", "sv")
assign.if.undef ("ARCH", "arn")
assign.if.undef ("METRIC", "Time")
assign.if.undef ("SAVE.PDF", FALSE)

# Check parameters
stopifnot (METRIC %in% c ("Time", "Mispredictions", "Branches", "Instructions"))

#======================================================================
# Load and transform data
#======================================================================

Data.set <- load.xform.many (ALG, ARCH, GRAPHS)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

#======================================================================
# Plot(s)
#======================================================================

# Copy of data for plotting purposes
Data.plot <- Data

# Choose x-variable
Data.plot <- transform (Data.plot, X=(Iters + 1) / Iters.bry)
x.label <- "Iterations"
x.label.sub <- "(normalized by branch-based algorithm)"
x.scale <- gen.axis.scale.auto (Data.plot$X, "x", scale="linear")

# Choose y-variable
if (METRIC == "Time") {
  Data.plot <- transform (Data.plot, Y=Time.cumul / Time.bry)
} else if (METRIC == "Mispredictions") {
  Data.plot <- transform (Data.plot, Y=Mispreds.cumul / Mispreds.bry)
} else if (METRIC == "Branches") {
  Data.plot <- transform (Data.plot, Y=Brs.cumul / Brs.bry)
} else if (METRIC == "Instructions") {
  Data.plot <- transform (Data.plot, Y=Insts.cumul / Insts.bry)
}
y.label <- sprintf ("Cumulative %s [%s]", METRIC, ARCH)
y.label.sub <- "(relative to branch-based algorithm)"
y.scale <- gen.axis.scale.auto (Data.plot$Y, "y", scale="linear")

# Apply subplot reordering
Order.by.Speedup <- rev (order (Summary$Speedup))
Data.plot$Graph <- with (Data.plot, factor (Graph, levels=levels (Graph)[Order.by.Speedup]))

# Generate plot
Q <- ggplot (Data.plot, aes (x=X, y=Y))
Q <- Q + geom_point (aes (colour=Alg, shape=Alg))
Q <- Q + geom_line (aes (colour=Alg, linetype=Alg))
Q <- Q + geom_hline (aes (yintercept=1), linetype="dashed", alpha=0.5)

Q <- Q + x.scale
Q <- add.title.optsub (Q, xlab, x.label, x.label.sub)

Q <- Q + y.scale
Q <- Q + ylab ("")
Q <- add.title.optsub (Q, ggtitle, y.label, sub=y.label.sub)
Q <- Q + theme (plot.title=element_text (hjust=-.025)) # left-align title

Q <- Q + facet_wrap (~ Graph)

Q <- Q + theme (legend.position="bottom")
Q <- Q + theme (legend.title=element_blank ())

Q <- Q + coord_equal (ratio=1)

Q <- set.hpcgarage.colours (Q)

#setDevPage.portrait ()
setDevSlide ()
print (Q)

if (SAVE.PDF) {
  outfilename <- sprintf ("figs/cumul-vs-iters--%s--%s--%s.pdf"
                          , METRIC
                          , ALG
                          , ARCH
                          )
  cat (sprintf ("Saving: %s ...\n", outfilename))
  setDevSlide.pdf (outfilename, l=15)
  ggsave (outfilename)
}

# eof
