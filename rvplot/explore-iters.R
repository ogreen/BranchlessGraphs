#======================================================================
# Use this script to explore "metric vs. iteration" plots.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

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

stopifnot (METRIC %in% Vars$Data)

#======================================================================
# Plot

alg.fancy <- ALGS.FANCY.MAP[[ALG]]
iter.tag <- if (ALG == "SV") "Iteration" else "Level"

# Determine output(s)
outfile.suffix <- get.file.suffix (ARCHS, ALG, CODES)
outfilename.iters <- sprintf ("figs2/explore-iters--%s--%s.pdf", METRIC, outfile.suffix)
cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  Metric vs. iterations (or levels): %s\n", outfilename.iters))

Df.plot <- Df[, Vars$Index]
Df.plot$Iteration <- Df.plot$Iteration + 1 # Change iteration from 0-based to 1-based index
x.label <- iter.tag
x.scale <- gen.axis.scale.auto (Df.plot$X, "x", scale=X.AXIS, free=FREE.SCALES)

Df.plot$Y <- as.numeric (Df[[METRIC]])
y.label <- METRIC
y.scale <- gen.axis.scale.auto (Df.plot$Y, "y", scale=Y.AXIS, free=FREE.SCALES)

stopifnot (FALSE)

if (CUMULATIVE) {
  # Wrong: overwrites Df.plot
  Df.plot <- ddply (Df.plot, Vars$Select.fit, summarise, Y.cumul=cumsum (Y))
}


Q.iters.display <- set.all.font.sizes (Q.iters, base=10)
Q.iters.pdf <- set.all.font.sizes (Q.iters, base=12)

if (!BATCH) {
  do.mpb <- prompt.yes.no ("\nDisplay misprediction bounds? ")
  if (do.mpb) {
    setDevPage.portrait ()
    print (Q.iters)
    pause.for.enter ()
  }
  do.mpb.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.mpb))
} else {
  do.mpb.pdf <- SAVE.PDF
}

if (do.mpb.pdf) {
  cat (sprintf ("\n--> Writing '%s' ... ", outfilename.mpb))
  setDevPage.portrait.pdf (outfilename.mpb, l=10)
  print (Q.iters.pdf)
  dev.off ()
  cat ("done!\n\n")
}

#======================================================================
# eof
