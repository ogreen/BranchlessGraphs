#======================================================================
# This script plots per-iteration metrics, relative to the
# branch-based algorithm for all input graphs given a single algorithm
# and architecture.
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("COMPUTATION", "sv")
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

Data.set <- load.xform.many (COMPUTATION, ARCHITECTURES, GRAPHS)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

#======================================================================
# Plot(s)
#======================================================================

cat (sprintf ("Plotting...\n"))

if (COMPUTATION == "sv") {
  alg.display.tag <- "Shiloach-Vishkin Connected Components"
} else {
  alg.display.tag <- "Top-down Breadth-First Search"
}

if (length (ARCHITECTURES) == length (ARCHS)) {
  arch.file.tag <- "All"
  arch.display.tag <- " [All platforms]"
} else {
  arch.file.tag <- paste (ARCHITECTURES, collapse="-")
  if (length (ARCHITECTURES) == 1) {
    arch.display.tag <- sprintf (" [%s]", ARCHITECTURES)
  } else {
    arch.display.tag <- ""
  }
}

X.SCALE <- if (SCALE %in% c ("xy", "logy")) "linear" else "log2"
Y.SCALE <- if (SCALE %in% c ("xy", "logx")) "linear" else "log2"
  
# Copy of data for plotting purposes
Data.plot <- Data

# Choose x-variable
Data.plot <- transform (Data.plot, X=(Iters + 1) / Iters.tot.bry)
x.label <- "Iterations"
x.label.sub <- "(normalized by branch-based algorithm)"
x.scale <- gen.axis.scale.auto (Data.plot$X, "x", scale=X.SCALE)

# Choose y-variable
if (METRIC == "Time") {
  Data.plot <- transform (Data.plot, Y.base=if (CUMULATIVE) Time.cumul else Time)
  Data.plot <- transform (Data.plot, Y.norm=if (CUMULATIVE) Time.tot.bry else Time.min.bry)
} else if (METRIC == "Mispredictions") {
  Data.plot <- transform (Data.plot, Y.base=if (CUMULATIVE) Mispreds.cumul else Mispreds)
  Data.plot <- transform (Data.plot, Y.norm=if (CUMULATIVE) Mispreds.tot.bry else Mispreds.min.bry)
} else if (METRIC == "Branches") {
  Data.plot <- transform (Data.plot, Y.base=if (CUMULATIVE) Brs.cumul else Brs)
  Data.plot <- transform (Data.plot, Y.norm=if (CUMULATIVE) Brs.tot.bry else Brs.min.bry)
} else if (METRIC == "Instructions") {
  Data.plot <- transform (Data.plot, Y.base=if (CUMULATIVE) Insts.cumul else Insts)
  Data.plot <- transform (Data.plot, Y.norm=if (CUMULATIVE) Insts.tot.bry else Insts.min.bry)
}
Data.plot <- transform (Data.plot, Y=Y.base / Y.norm)
y.label <- sprintf ("%s: %s%s%s"
                    , alg.display.tag
                    , if (CUMULATIVE) "Cumulative " else ""
                    , METRIC
                    , arch.display.tag)
y.label.sub <- "(relative to branch-based algorithm)"
y.scale <- gen.axis.scale.auto (Data.plot$Y, "y", scale=Y.SCALE)

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

if (length (ARCHITECTURES) == 1) {
  Q <- Q + facet_wrap (~ Graph)
} else {
  Q <- Q + facet_grid (Arch ~ Graph)
}

Q <- Q + theme (legend.position="bottom")
Q <- Q + theme (legend.title=element_blank ())

#Q <- Q + coord_equal (ratio=1)

Q <- set.hpcgarage.colours (Q)

# Screen output
#setDevPage.portrait ()
setDevSlide ()
print (Q)

# Print copy, if requested
outfilename <- sprintf ("figs/%s%s-vs-iters--%s--%s.pdf"
                        , if (CUMULATIVE) "cumul-" else ""
                        , METRIC
                        , COMPUTATION
                        , arch.file.tag
                        )
if (!SAVE.PDF) {
  cat (sprintf ("[Output file (not saving): %s]\n", outfilename))
} else {
  cat (sprintf ("Saving: %s ...\n", outfilename))
  setDevSlide.pdf (outfilename, l=15)
  ggsave (outfilename)
}

# eof
