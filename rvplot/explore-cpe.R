#======================================================================
# Use this script to plot and compare cycles per edge (CPE), measured
# against modeled.

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

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot (all (ALGS %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALGS, CODES)
outfilename.cpe <- sprintf ("figs2/explore-cpe--%s.pdf", outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  CPE: %s\n", outfilename.cpe))

#======================================================================
# Preprocess data

Df.arch <- get.perfdf.arch (All.data, ARCH, ALGS, CODES)
Vars.arch <- get.perfdf.var.info (Df.arch, All.data)

cat ("Computing per-iteration data normalized by edges traversed ...\n")
Norm <- get.perfdf.norm (Df.arch, Vars.arch, by="Edges")
Df.arch.norm <- normalize.perfdf (Df.arch, Vars.arch, Norm)

cat ("Aggregating totals ...\n")
Df.arch.tot <- total.perfdf (Df.arch, Vars.arch)

cat ("Normalizing totals by edges traversed ...\n")
Norm.tot <- get.perfdf.norm (Df.arch.tot, Vars.arch, by="Edges")
Df.arch.tot.norm <- normalize.perfdf (Df.arch.tot, Vars.arch, Norm.tot)

#======================================================================
# Build models of the data

cat (sprintf ("Building models ...\n"))

Data.predicted <- NULL
Predictions <- NULL
response.var <- if (Vars.arch$has.cycles) "Cycles" else "Time"
response.true <- sprintf ("%s.true", response.var)

arch <- ARCH
for (alg in ALGS) {
  for (code in CODES) {
    cat (sprintf ("==> %s for %s on %s ...\n", code, alg, arch))
    
    # Choose subset of data to fit
#    Data.fit <- subset (Df.arch.tot.norm, Algorithm == alg & Implementation == code)
    Data.fit <- subset (Df.arch.norm, Algorithm == alg & Implementation == code)
    
    # Determine predictors
    vars.key <- get.file.suffix (arch, alg, code)
    vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
    if (!file.exists (vars.file)) {
      stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
    }
    Predictors <- as.vector (unlist (read.table (vars.file)))
    Predictors <- c (Predictors, "Instructions")

    # Fit! Use nonnegative least squares without a constant term
    Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                           , constant.term=FALSE, nonneg=TRUE)

    cat (sprintf ("\n=== Fitted model for: %s code for %s on %s ===\n", code, alg, arch))
    print (summary (Fit))

    # Use model to predict totals
    Data.predict <- subset (Df.arch.tot.norm, Algorithm == alg & Implementation == code)
    Prediction <- predict.df.lm (Fit, Data.predict, response.var)
    Prediction[, response.true] <- Data.predict[, response.var]
    Prediction <- cbind (Data.predict[, Vars.arch$Index], Prediction)

    cat (sprintf ("\n=== Sample predictions ===\n"))
    print (head (Prediction))

    Predictions <- rbind.fill (Predictions, Prediction)
    Data.predicted <- rbind.fill (Data.predicted, Data.predict)
  } # for each code
} # for each alg

#======================================================================
# Plot

Fixed.cols <- c (Vars.arch$Index, response.var, response.true)
Predictions.FV <- split.df.by.colnames (Predictions, Fixed.cols)
Predictions.flat <- flatten.keyvals.df (Predictions.FV$A, Predictions.FV$B)

Y.predicted <- Predictions[[response.var]]
Y.true <- Predictions[[response.true]]
Other.values <- Predictions.flat$Value
Y.values <- with (Predictions, c (Y.predicted, Y.true, Other.values))

Q.cpe <- qplot (Graph, Value, data=Predictions.flat, geom="bar", stat="identity", fill=Key)

Q.cpe <- set.hpcgarage.fill (Q.cpe, name="Predicted values: ")
Q.cpe <- Q.cpe + theme (legend.position="bottom")
Q.cpe <- Q.cpe + xlab ("") + ylab ("") # Erase default labels

# Add measured values
Q.cpe <- Q.cpe + geom_point (aes (x=Graph, y=Y.true), data=Data.predicted
                             , colour="black", fill=NA, shape=18, size=4)

Q.cpe <- Q.cpe + facet_grid (Algorithm ~ Implementation, scales="free_y")

Q.cpe <- Q.cpe + theme(axis.text.x=element_text(angle=35, hjust = 1))

title.str <- sprintf ("Predicted %s per Edge Traversed [%s]", response.var, arch)
Q.cpe <- add.title.optsub (Q.cpe, ggtitle, main=title.str)
#Q.cpe <- Q.cpe + gen.axis.scale.auto (Y.values, "y")

Q.cpe.display <- set.all.font.sizes (Q.cpe, base=10)
Q.cpe.pdf <- set.all.font.sizes (Q.cpe, base=12)

if (!BATCH) {
  do.cpe <- prompt.yes.no ("\nDisplay CPE? ")
  if (do.cpe) {
    setDevHD ()
    print (Q.cpe)
    pause.for.enter ()
  }
  do.cpe.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.cpe))
} else {
  do.cpe.pdf <- SAVE.PDF
}

if (do.cpe.pdf) {
  cat (sprintf ("\n--> Writing '%s' ... ", outfilename.cpe))
  setDevHD.pdf (outfilename.cpe, l=18)
  print (Q.cpe.pdf)
  dev.off ()
  cat ("done!\n\n")
}

#======================================================================
# eof
