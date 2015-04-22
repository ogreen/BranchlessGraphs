#======================================================================
# Use this script to explore "metric vs. iteration" plots.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")

# Load data, if not already available
assign.if.undef ("All.data", load.perfdata.many ())
All.algs <- unique (get.all.colvals (All.data, "Algorithm"))
All.codes <- unique (get.all.colvals (All.data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)
assign.if.undef ("CONCISE.GRAPHS", TRUE)
assign.if.undef ("SHOW.SPEEDUP", TRUE) # Enable speedup annotation in each subplot

GRAPHS.DEFAULT <- if (CONCISE.GRAPHS) GRAPHS.CONCISE else GRAPHS.ALL

if (!BATCH) {
  prompt.any.if.undef ("ARCHS", keyword="architectures", ARCHS.ALL.MAP)
  prompt.if.undef ("ALG", keyword="algorithms", All.algs)
  prompt.any.if.undef ("CODES", keyword="implementations", All.codes)
  prompt.any.if.undef ("GRAPHS", keyword="input graphs", GRAPHS.DEFAULT)
} else {
  assign.if.undef ("ARCHS", as.vector (unlist (ARCHS.ALL.MAP)))
  assign.if.undef ("ALG", "SV")
  assign.if.undef ("CODES", as.vector (unlist (CODES.ALL.MAP)))
  assign.if.undef ("GRAPHS", GRAPHS.DEFAULT)
}

# Check above configuration parameters
stopifnot (all (ARCHS %in% ARCHS.ALL.MAP))
stopifnot ((length (ALG) == 1) & (ALG %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))
stopifnot (all (GRAPHS %in% GRAPHS.DEFAULT))

#======================================================================
# Preprocess data

Df <- get.perfdf (All.data, ARCHS, ALG, CODES)
Df <- subset (Df, Graph %in% GRAPHS) # Show data for only some graphs
Vars <- get.perfdf.var.info (Df, All.data)

cat ("Aggregating totals ...\n")
Df.tot <- total.perfdf (Df, Vars)
Df.tot.bb <- subset (Df.tot, Implementation == "Branch-based")
Df.tot.bb$Implementation <- NULL # remove
Vars.tot.merge <- setdiff (Vars$Select.fit, "Implementation")
Df.tot <- merge (Df.tot, Df.tot.bb, by=Vars.tot.merge, suffixes=c ("", ".bb"))

#======================================================================
# Analysis

cat ("\n")

# Determine what to analyze
if (!BATCH) {
  prompt.if.undef ("METRIC", keyword="metrics to plot", Vars$Data)
} else {
  assign.if.undef ("METRIC", "Time")
}
assign.if.undef ("CUMULATIVE", FALSE)
assign.if.undef ("X.AXIS", "linear")
assign.if.undef ("Y.AXIS", "linear")
assign.if.undef ("FREE.SCALES", TRUE)
assign.if.undef ("NORMALIZE", TRUE)

stopifnot (METRIC %in% Vars$Data)

#======================================================================
# Plot

alg.fancy <- ALGS.FANCY.MAP[[ALG]]
iter.tag <- if (ALG == "SV") "Iteration" else "Level"
cumul.tag <- if (CUMULATIVE) "Cumulative " else ""
title.tag <- sprintf ("%s: %s%s", alg.fancy, cumul.tag, METRIC)

if (NORMALIZE) {
  norm.tag <- sprintf ("[Normalized to branch-based %s]"
                       , if (CUMULATIVE) "total" else "minimum")
} else {
  norm.tag <- NA
}

# Determine output(s)
outfile.suffix <- get.file.suffix (ARCHS, ALG, CODES)
outfilename.iters <- sprintf ("figs2/explore-iters%s--%s--%s.pdf"
                              , if (CUMULATIVE) "-cumul" else ""
                              , METRIC
                              , outfile.suffix
                              )
cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  Metric vs. iterations (or levels): %s\n", outfilename.iters))

Df.plot <- Df[, Vars$Index]
Df.plot$X <- Df.plot$Iteration + 1 # Change iteration from 0-based to 1-based index
x.label <- iter.tag
x.scale <- gen.axis.scale.auto (Df.plot$X, "x", scale=X.AXIS, free=FREE.SCALES)

Df.plot$Y <- as.numeric (Df[[METRIC]])
y.label <- METRIC
y.scale <- gen.axis.scale.auto (Df.plot$Y, "y", scale=Y.AXIS, free=FREE.SCALES)

if (CUMULATIVE) {
  Df.plot <- ddply (Df.plot, Vars$Select.fit, transform, Iteration=Iteration
                    , X=Iteration, Y=cumsum (Y))
}

if (NORMALIZE) {
  f.norm <- if (CUMULATIVE) max else min
  Df.norm <- aggregate.perfdf (Df.plot, Vars, f.norm)[, c (Vars$Select.fit, "Y")]
  Df.norm <- subset (Df.norm, Implementation == "Branch-based")
  Vars.norm <- setdiff (Vars$Select.fit, "Implementation")
  Df.plot <- merge (Df.plot, Df.norm, by=Vars.norm, suffixes=c ("", ".norm"))
  Df.plot$Y <- with (Df.plot, Y / Y.norm)
}

Q.iters <- ggplot (Df.plot, aes (x=X, y=Y))
Q.iters <- Q.iters + geom_point (aes (colour=Implementation, shape=Implementation), size=2)
Q.iters <- Q.iters + geom_line (aes (colour=Implementation))
Q.iters <- Q.iters + x.scale
Q.iters <- Q.iters + y.scale
Q.iters <- Q.iters + facet_grid (Architecture ~ Graph, scales=if (FREE.SCALES) "free" else "fixed")

Q.iters <- Q.iters + geom_hline (yintercept=1, linetype="dashed", alpha=0.5)

# For each subplot, add a speedup label
if (SHOW.SPEEDUP) {
  # Compute label
  Df.speedup <- subset (Df.tot, Implementation != "Branch-based")
  var.baseline <- sprintf ("%s.bb", METRIC)
  Df.speedup$Speedup <- Df.speedup[[var.baseline]] / Df.speedup[[METRIC]]
  Df.speedup <- Df.speedup[, c (Vars$Select.fit, "Iteration", "Speedup")]
  Df.speedup$Label <- sprintf ("%.2fx", Df.speedup$Speedup)

  # Compute label position
  Vars.x <- setdiff (Vars$Select.fit, c ("Architecture", "Implementation"))
  Df.pos.x <- ddply (Df.plot, Vars.x, summarise, X=if (CUMULATIVE) min (X) else max (X))
  Vars.y <- setdiff (Vars$Select.fit, c ("Graph", "Implementation"))
  Df.pos.y <- ddply (Df.plot, Vars.y, summarise, Y=max (Y))
  Df.speedup <- merge (Df.speedup, Df.pos.x, by=Vars.x, suffixes=c ("", ".x"))
  Df.speedup <- merge (Df.speedup, Df.pos.y, by=Vars.y, suffixes=c ("", ".y"))
  
  # Add annotations to plot
  Q.iters <- Q.iters + geom_text (data=Df.speedup
                                  , aes (label=Label, colour=Implementation)
                                  , hjust=if (CUMULATIVE) 0 else 1
                                  , vjust=1)
}

Q.iters <- add.title.optsub (Q.iters, ggtitle, main=title.tag, sub=norm.tag)
Q.iters <- Q.iters + xlab (iter.tag)
Q.iters <- Q.iters + ylab ("")
Q.iters <- Q.iters + theme (legend.position="bottom")
Q.iters <- set.hpcgarage.colours (Q.iters)

Q.iters.display <- set.all.font.sizes (Q.iters, base=10)
Q.iters.pdf <- set.all.font.sizes (Q.iters, base=12)

if (!BATCH) {
  do.iters <- prompt.yes.no (sprintf ("\nDisplay %s vs. iterations plot? ", tolower (METRIC)))
  if (do.iters) {
    setDevHD ()
    print (Q.iters.display)
  }
  do.iters.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.iters))
} else {
  do.iters.pdf <- SAVE.PDF
}

if (do.iters.pdf) {
  cat (sprintf ("\n--> Writing '%s' ... ", outfilename.iters))
  setDevHD.pdf (outfilename.iters, l=15)
  print (Q.iters.pdf)
  dev.off ()
  cat ("done!\n\n")
}

#======================================================================
# eof
