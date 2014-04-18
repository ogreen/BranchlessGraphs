#======================================================================
# This script plots per-iteration metrics, relative to the
# branch-based algorithm for all input graphs given a single algorithm
# and architecture.
#
# For a sample plot, see: figs/HD--Time-vs-iters--sv--All.pdf
# ======================================================================

source ("rvplot-inc.R")

#======================================================================
# Script parameters
#======================================================================

assign.if.undef ("COMP", "sv") # "sv" or "bfs"
assign.if.undef ("ARCHS", NA) # specific platforms, or NA for all
assign.if.undef ("METRIC", "Time") # "Time", "Mispredictions", "Branches", or "Instructions"
assign.if.undef ("FREE.SCALES", TRUE) # FALSE for fixed axis values, TRUE for flexible ones
assign.if.undef ("PER.EDGE", FALSE) # Normalize per edge traversed
assign.if.undef ("CUMULATIVE", FALSE) # Use cumulative plots
assign.if.undef ("AXES", "xy") # linear or log axes: "xy", "logx", "logy", or "loglog"
assign.if.undef ("SHOW.POINTS", TRUE)
assign.if.undef ("SHOW.LINES", TRUE)
assign.if.undef ("SHOW.SPEEDUP", TRUE) # Enable speedup annotation in each subplot
assign.if.undef ("HD", TRUE) # 16 x 9 instead of 4 x 3
assign.if.undef ("SAVE.PDF", FALSE) # Write to PDF

# Check parameters
stopifnot (COMP %in% COMPS.ALL)
if (all (is.na (ARCHS))) { # by default, plot all architectures
  ARCHS <- ARCHS.ALL
}
stopifnot (METRIC %in% c ("Time", "Mispredictions", "Branches", "Instructions"))
stopifnot (AXES %in% c ("xy", "logx", "logy", "loglog"))

#======================================================================
# Load and transform data
#======================================================================

Data.set <- load.xform.many (COMP, ARCHS, GRAPHS.ALL)
Data <- Data.set[["Data"]]
Summary <- Data.set[["Summary"]]

Annotations <- Summary[, c ("Comp", "Arch", "Graph"
                            , "Speedup", "Mispreds.ratio", "Brs.ratio", "Insts.ratio"
                            , "Iters.tot.bry", "Iters.tot.brl"
                            )]
Annotations <- transform (Annotations, Iters.max=pmax (Iters.tot.bry, Iters.tot.brl))
Annotations$Iters.tot.bry <- NULL
Annotations$Iters.tot.brl <- NULL

#======================================================================
# Plot(s)
#======================================================================

cat (sprintf ("Plotting...\n"))

if (COMP == "sv") {
  alg.display.tag <- "Shiloach-Vishkin Connected Components"
} else {
  alg.display.tag <- "Top-down Breadth-First Search"
}

if (length (ARCHS) == length (ARCHS.ALL)) {
  arch.file.tag <- "All"
  arch.display.tag <- " [All platforms]"
} else {
  arch.file.tag <- paste (ARCHS, collapse="-")
  if (length (ARCHS) == 1) {
    arch.display.tag <- sprintf (" [%s]", ARCHS)
  } else {
    arch.display.tag <- ""
  }
}

X.AXIS <- if (AXES %in% c ("xy", "logy")) "linear" else "log2"
Y.AXIS <- if (AXES %in% c ("xy", "logx")) "linear" else "log2"
  
# Copy of data for plotting purposes
Data.plot <- Data
Annotations.plot <- Annotations

# Choose x-variable
Data.plot <- transform (Data.plot, X=Iters + 1)
x.label <- "Iterations"
x.label.sub <- ""
x.scale <- gen.axis.scale.auto (Data.plot$X, "x", scale=X.AXIS, free=FREE.SCALES)

# Choose y-variable
if (METRIC == "Time") {
  Data.plot <- transform (Data.plot
                          , Y.iter=Time
                          , Y.cumul=Time.cumul
                          , Y.edge=Time.E
                          , Y.tot.bry=Time.tot.bry
                          , Y.min.bry=Time.min.bry)
  Annotations.plot <- transform (Annotations.plot, S=Speedup)
} else if (METRIC == "Mispredictions") {
  Data.plot <- transform (Data.plot
                          , Y.iter=Mispreds
                          , Y.cumul=Mispreds.cumul
                          , Y.edge=Mispreds.E
                          , Y.tot.bry=Mispreds.tot.bry
                          , Y.min.bry=Mispreds.min.bry)
  Annotations.plot <- transform (Annotations.plot, S=Mispreds.ratio)
} else if (METRIC == "Branches") {
  Data.plot <- transform (Data.plot
                          , Y.iter=Brs
                          , Y.cumul=Brs.cumul
                          , Y.edge=Brs.E
                          , Y.tot.bry=Brs.tot.bry
                          , Y.min.bry=Brs.min.bry)
  Annotations.plot <- transform (Annotations.plot, S=Brs.ratio)
} else if (METRIC == "Instructions") {
  Data.plot <- transform (Data.plot
                          , Y.iter=Insts
                          , Y.cumul=Insts.cumul
                          , Y.edge=Insts.E
                          , Y.tot.bry=Insts.tot.bry
                          , Y.min.bry=Insts.min.bry)
  Annotations.plot <- transform (Annotations.plot, S=Insts.ratio)
}

prefix.tag <- ""
norm.tag <- ""
suffix.tag <- ""
if (CUMULATIVE) {
  # Cumulative value, normalized by total value in branchy case
  Data.plot <- transform (Data.plot, Y=Y.cumul / Y.tot.bry)
  prefix.tag <- "Cumulative "
  norm.tag <- "(relative to total for branch-based algorithm)"
} else if (PER.EDGE) {
  # Value per edge traversal, unnormalized
  Data.plot <- transform (Data.plot, Y=Y.edge)
  suffix.tag <- " per edge traversal"
} else {
  # Value per iteration, normalized by minimum value per iteration in branchy case
  Data.plot <- transform (Data.plot, Y=Y.iter / Y.min.bry)
  suffix.tag <- " per iteration"
  norm.tag <- "(relative to minimum in any iteration of branch-based algorithm)"
}
y.label <- sprintf ("%s: %s%s%s%s"
                    , alg.display.tag
                    , prefix.tag
                    , METRIC
                    , suffix.tag
                    , arch.display.tag)
y.label.sub <- norm.tag
y.scale <- gen.axis.scale.auto (Data.plot$Y, "y", scale=Y.AXIS, free=FREE.SCALES)

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

facet.scale <- if (FREE.SCALES) "free" else "fixed"
if (length (ARCHS) == 1) {
  Q <- Q + facet_wrap (~ Graph, scale=facet.scale)
} else {
  Q <- Q + facet_grid (Arch ~ Graph, scales=facet.scale)
}

# For each subplot, add a speedup label
if (SHOW.SPEEDUP) {
  Y.pos <- ddply (Data.plot[, c ("Comp", "Arch", "Y")], .(Comp, Arch), summarise
                  , Max=max (Y), Min=min (Y))
  Annotations.plot <- merge (Annotations.plot, Y.pos, by=c ("Comp", "Arch"))
  Q <- Q + geom_text (data=Annotations.plot
                      , aes (x=Iters.max, y=Min/4 + Max*3/4, label=sprintf ("%.2fx", S))
                      , hjust=1, vjust=1, size=4)
}

Q <- Q + theme (legend.position="bottom")
Q <- Q + theme (legend.title=element_blank ())

#Q <- Q + coord_equal (ratio=1)

Q <- set.hpcgarage.colours (Q)

# Screen output
#setDevPage.portrait ()
if (HD) {
  setDevHD ()
} else {
  setDevSlide ()
}
print (Q)

# Print copy, if requested
outfilename <- sprintf ("figs/%s%s%s%s-vs-iters--%s--%s.pdf"
                        , if (HD) "HD--" else ""
                        , if (CUMULATIVE) "cumul-" else ""
                        , METRIC
                        , if (PER.EDGE) "-per_edge" else ""
                        , COMP
                        , arch.file.tag
                        )
if (!SAVE.PDF) {
  cat (sprintf ("[Output file (not saving): %s]\n", outfilename))
} else {
  cat (sprintf ("Saving: %s ...\n", outfilename))
  if (HD) {
    setDevHD.pdf (outfilename, l=15)
  } else {
    setDevSlide.pdf (outfilename, l=15)
  }
  print (Q)
  dev.off ()
}

# eof
