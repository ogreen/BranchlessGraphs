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

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot (all (ALGS %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))
stopifnot (all (GRAPHS %in% GRAPHS.ALL))
stopifnot (FACET.COL %in% c ("Graph", "Implementation"))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALGS, CODES)
outfilename.cpi <- sprintf ("figs2/explore-cpi%s--%s.pdf"
                            , if (CONST.TERM) "-const" else ""
                            , outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  CPI: %s\n", outfilename.cpi))

#======================================================================
# Preprocess data

Df <- subset (get.perfdf (All.data, ARCH, ALGS, CODES), Graph %in% GRAPHS)
Vars <- get.perfdf.var.info (Df, All.data)

cat ("Computing per-iteration data normalized by instructions ...\n")
Inst.norm <- get.perfdf.norm (Df, Vars, by="Instructions")
Df.per.inst <- normalize.perfdf (Df, Vars, Inst.norm)

cat ("Aggregating totals ...\n")
Df.tot <- total.perfdf (Df, Vars)

cat ("Normalizing totals by instructions ...\n")
Inst.tot.norm <- get.perfdf.norm (Df.tot, Vars, by="Instructions")
Df.tot.per.inst <- normalize.perfdf (Df.tot, Vars, Inst.tot.norm)

#======================================================================
# Build models of the data

cat (sprintf ("Building models ...\n"))

source ("fit-cpi-inc.R")

Fits <- fit.over.all.graphs (Vars, Df.fit=Df.per.inst, Df.predict=Df.tot.per.inst
#Fits <- fit.one.per.graph (Vars, Df.fit=Df.per.inst, Df.predict=Df.tot.per.inst
                           , const.term=CONST.TERM, nonneg=TRUE)
Data.predicted <- Fits$Data.predicted
Predictions <- Fits$Predictions
response.var <- Fits$response.var
response.true <- Fits$response.true

# Dump models
Predictors.all <- Fits$Predictors.all
Models.all <- Fits$Models
for (mod.key in names (Models.all)) {
  model <- Models.all[[mod.key]]
  preds <- Predictors.all[[mod.key]]
  if (CONST.TERM) { preds <- c ("(Intercept)", preds) }
  if ("nnlm" %in% class (model)) {
    Model.summary <- data.frame (Predictor=preds, Coef=model$model$x)
  } else {
    Model.summary <- data.frame (Predictor=preds, Coef=as.vector (model[["coefficients"]]))
  }
  cat (sprintf ("\n=== Model: %s [%s] ===\n", mod.key, class (model)))
  print (Model.summary)
  cat (sprintf ("\nR^2: %g\n", model$res2))
}

#======================================================================
# Plot

Fixed.cols <- c (Vars$Index, response.var, response.true)
Predictions.FV <- split.df.by.colnames (Predictions, Fixed.cols)
Predictions.flat <- flatten.keyvals.df (Predictions.FV$A, Predictions.FV$B)

Y.predicted <- Predictions[[response.var]]
Y.true <- Predictions[[response.true]]
Other.values <- Predictions.flat$Value
Y.values <- with (Predictions, c (Y.predicted, Y.true, Other.values))

if (FACET.COL == "Implementation") {
  Predictions.flat$X <- Predictions.flat$Graph
  Predictions.flat$Group <- Predictions.flat$Implementation
  Predictions$X <- Predictions$Graph
  Predictions$Group <- Predictions$Implementation
} else {
  stopifnot (FACET.COL == "Graph")
  Predictions.flat$X <- Predictions.flat$Implementation
  Predictions$X <- Predictions$Implementation
  Predictions.flat$Group <- Predictions.flat$Graph
  Predictions$Group <- Predictions$Graph
}  

Q.cpi <- qplot (X, Value, data=Predictions.flat, geom="bar", stat="identity", fill=Key)
Q.cpi <- Q.cpi + geom_point (aes (x=X, y=Cycles.true), data=Predictions
                             , colour="black", fill=NA, shape=18, size=4)
Q.cpi <- Q.cpi + facet_grid (Algorithm ~ Group, scales="free_y")

Q.cpi <- Q.cpi + theme(axis.text.x=element_text(angle=35, hjust = 1))

Q.cpi <- set.hpcgarage.fill (Q.cpi, name="Predicted values: ")
Q.cpi <- Q.cpi + theme (legend.position="bottom")
Q.cpi <- Q.cpi + xlab ("") + ylab ("") # Erase default labels

title.str <- sprintf ("Predicted %s per instruction [%s]", response.var, ARCH)
Q.cpi <- add.title.optsub (Q.cpi, ggtitle, main=title.str)
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
