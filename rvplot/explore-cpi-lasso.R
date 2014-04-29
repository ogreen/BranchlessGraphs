#======================================================================
# Use this script to plot and compare CPI, measured against modeled.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

# Load data, if not already available
assign.if.undef ("All.data", load.perfdata.many ())
All.codes <- unique (get.all.colvals (All.data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

if (!BATCH) {
  prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL.MAP)
  prompt.any.if.undef ("ALGS", keyword="algorithms", unique (All.data[[ARCH]]$Algorithm))
  prompt.any.if.undef ("CODES", keyword="implementations", unique (All.data[[ARCH]]$Implementation))
} else {
  assign.if.undef ("ARCH", "Haswell")
  assign.if.undef ("ALGS", as.vector (unlist (ALGS.ALL.MAP)))
  assign.if.undef ("CODES", as.vector (unlist (CODES.ALL.MAP)))
}
assign.if.undef ("GRAPHS", GRAPHS.ALL)
assign.if.undef ("CONST.TERM", FALSE)
assign.if.undef ("NONNEG", TRUE)
assign.if.undef ("FACET.COL", "Graph") # or, "Implementation"
assign.if.undef ("FIT.PER.GRAPH", FALSE)

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot (all (ALGS %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))
stopifnot (all (GRAPHS %in% GRAPHS.ALL))
stopifnot (FACET.COL %in% c ("Graph", "Implementation"))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALGS, CODES)
outfilename.cpi <- sprintf ("figs2/explore-cpi-lasso%s%s--%s.pdf"
                            , if (FIT.PER.GRAPH) "-per_graph" else ""
                            , if (CONST.TERM) "-const" else ""
                            , outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  CPI: %s\n", outfilename.cpi))

#======================================================================
# Preprocess data

Df <- subset (get.perfdf (All.data, ARCH, ALGS, CODES), Graph %in% GRAPHS)
Vars <- get.perfdf.var.info (Df, All.data)

cat ("Computing per-iteration data normalized by branch-based instructions ...\n")
Inst.norm <- get.perfdf.norm (Df, Vars, by="Instructions", use.branch.based=TRUE)
Df.per.inst <- normalize.perfdf (Df, Vars, Inst.norm)

cat ("Aggregating totals ...\n")
Df.tot <- total.perfdf (Df, Vars)

cat ("Normalizing totals by instructions ...\n")
Inst.tot.norm <- get.perfdf.norm (Df.tot, Vars, by="Instructions", use.branch.based=TRUE)
Df.tot.per.inst <- normalize.perfdf (Df.tot, Vars, Inst.tot.norm)

cat ("Computing speedups over the branch-based case ...\n")
Index.bb <- setdiff (Vars$Index, "Implementation")
Df.tot.bb <- subset (Df.tot, Implementation == "Branch-based")[, c (Index.bb, "Time")]
Df.speedup <- merge (Df.tot, Df.tot.bb, by=Index.bb, suffixes=c ("", ".bb"))
Df.speedup$Speedup <- with (Df.speedup, Time.bb / Time)

#======================================================================
# Build models of the data

cat (sprintf ("Building models ...\n"))

source ("fit-cpi-lasso-inc.R")

f.fit <- if (FIT.PER.GRAPH) fit.cpi.one.per.graph else fit.cpi.over.all.graphs
Fits <- f.fit (Vars, Df.fit=Df.per.inst, Df.predict=Df.tot.per.inst
               , const.term=CONST.TERM, nonneg=NONNEG)
Predictions <- Fits$Predictions
response.var <- Fits$response.var
response.true <- Fits$response.true

summary (Fits)

#======================================================================
# Plot

if (FACET.COL == "Implementation") {
  Predictions$X <- Predictions$Graph
  Predictions$Group <- Predictions$Implementation
} else {
  stopifnot (FACET.COL == "Graph")
  Predictions$X <- Predictions$Implementation
  Predictions$Group <- Predictions$Graph
}
Predictions$Y.true <- Predictions[, response.true]

Fixed.cols <- c (Vars$Index, response.var, response.true, "X", "Group")
Predictions.FV <- split.df.by.colnames (Predictions, Fixed.cols)
Predictions.flat <- flatten.keyvals.df (Predictions.FV$A, Predictions.FV$B)

Y.predicted <- Predictions[[response.var]]
Y.true <- Predictions[[response.true]]
Other.values <- Predictions.flat$Value
Y.values <- with (Predictions, c (Y.predicted, Y.true, Other.values))

# Filter to only the maximal set of non-zero predictors across all models
Keys.gcp <- get.gcp.cpi.lm.lasso (Fits)
Predictions.flat <- subset (Predictions.flat, Key %in% Keys.gcp)

Q.cpi <- qplot (X, Value, data=Predictions.flat, geom="bar", stat="identity", fill=Key)
Q.cpi <- Q.cpi + geom_point (aes (x=X, y=Y.true), data=Predictions
                             , colour="black", fill=NA, shape=18, size=4)
Q.cpi <- Q.cpi + facet_grid (Algorithm ~ Group, scales="free_y")

Q.cpi <- Q.cpi + theme(axis.text.x=element_text(angle=35, hjust = 1))

Q.cpi <- set.hpcgarage.fill (Q.cpi, name="Predicted values: ")
Q.cpi <- Q.cpi + theme (legend.position="bottom")
Q.cpi <- Q.cpi + xlab ("") + ylab ("") # Erase default labels

# Add speedup labels
Df.speedup.non.bb <- subset (Df.speedup, Implementation != "Branch-based")
Df.speedup.non.bb <- Df.speedup.non.bb[, c (Vars$Select.fit, "Speedup")]

Labels.non.bb <- subset (Predictions, Implementation != "Branch-based")
Labels.non.bb <- merge (Labels.non.bb, Df.speedup.non.bb
                        , by=Vars$Select.fit, suffixes=c ("", ".speedup"))
Labels.non.bb <- transform (Labels.non.bb
                            , Label=sprintf ("%sx", genSigFigLabels (Speedup, 2)))
Labels.non.bb$Y.true <- Labels.non.bb[[response.true]]

text.colour <- if (length (Keys.gcp) == 8) PAL.HPCGARAGE[["grey"]] else "black"
Q.cpi <- Q.cpi + geom_text (aes (x=X, y=Y.true, label=Label, fill=NA)
                            , colour=text.colour
                            , vjust=-1, size=4
                            , data=Labels.non.bb)

# Add titles
title.str <- sprintf ("Predicted %s per instruction [%s]", response.var, ARCH)
subtitle.str <- "[normalized by the number of *branch-based* instructions]"
Q.cpi <- add.title.optsub (Q.cpi, ggtitle, main=title.str, sub=subtitle.str)
#Q.cpi <- Q.cpi + gen.axis.scale.auto (Y.values, "y")

Q.cpi.display <- set.all.font.sizes (Q.cpi, base=10)
Q.cpi.pdf <- set.all.font.sizes (Q.cpi, base=12)

if (!BATCH) {
  do.cpi <- prompt.yes.no ("\nDisplay CPI? ")
  if (do.cpi) {
    setDevHD ()
    print (Q.cpi)
    pause.for.enter ()
  }
  do.cpi.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.cpi))
} else {
  do.cpi.pdf <- SAVE.PDF
}

if (do.cpi.pdf) {
  cat (sprintf ("\n--> Writing '%s' ... ", outfilename.cpi))
  setDevHD.pdf (outfilename.cpi, l=18)
  print (Q.cpi.pdf)
  dev.off ()
  cat ("done!\n\n")
}

#======================================================================
# eof
