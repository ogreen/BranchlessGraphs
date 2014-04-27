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
  CONST.TERM <- prompt.yes.no ("When fitting, include a constant term? ")
} else {
  assign.if.undef ("ARCH", "Haswell")
  assign.if.undef ("ALGS", as.vector (unlist (ALGS.ALL.MAP)))
  assign.if.undef ("CODES", as.vector (unlist (CODES.ALL.MAP)))
  assign.if.undef ("CONST.TERM", FALSE)
}

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot (all (ALGS %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))

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

Df.arch <- get.perfdf.arch (All.data, ARCH, ALGS, CODES)
Vars.arch <- get.perfdf.var.info (Df.arch, All.data)

cat ("Computing per-iteration data normalized by instructions ...\n")
Inst.norm <- get.perfdf.norm (Df.arch, Vars.arch, by="Instructions")
Df.arch.per.inst <- normalize.perfdf (Df.arch, Vars.arch, Inst.norm)

cat ("Aggregating totals ...\n")
Df.arch.tot <- total.perfdf (Df.arch, Vars.arch)

cat ("Normalizing totals by instructions ...\n")
Inst.tot.norm <- get.perfdf.norm (Df.arch.tot, Vars.arch, by="Instructions")
Df.arch.tot.per.inst <- normalize.perfdf (Df.arch.tot, Vars.arch, Inst.tot.norm)

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
#    Data.fit <- subset (Df.arch.tot.per.inst, Algorithm == alg & Implementation == code)
    Data.fit <- subset (Df.arch.per.inst, Algorithm == alg & Implementation == code)
    
    # Determine predictors
    vars.key <- get.file.suffix (arch, alg, code)
    vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
    if (!file.exists (vars.file)) {
      stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
    }
    Predictors <- as.vector (unlist (read.table (vars.file)))

    # Fit! Use nonnegative least squares without a constant term
    Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                           , constant.term=CONST.TERM, nonneg=TRUE)

    cat (sprintf ("\n=== Fitted model for: %s code for %s on %s ===\n", code, alg, arch))
    print (summary (Fit))

    # Use model to predict totals
    Data.predict <- subset (Df.arch.tot.per.inst, Algorithm == alg & Implementation == code)
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

Q.cpi <- qplot (Graph, Value, data=Predictions.flat, geom="bar", stat="identity", fill=Key)

Q.cpi <- set.hpcgarage.fill (Q.cpi, name="Predicted values: ")
Q.cpi <- Q.cpi + theme (legend.position="bottom")
Q.cpi <- Q.cpi + xlab ("") + ylab ("") # Erase default labels

# Add measured values
Q.cpi <- Q.cpi + geom_point (aes (x=Graph, y=Y.true), data=Data.predicted
                             , colour="black", fill=NA, shape=18, size=4)

Q.cpi <- Q.cpi + facet_grid (Algorithm ~ Implementation, scales="free_y")

Q.cpi <- Q.cpi + theme(axis.text.x=element_text(angle=35, hjust = 1))

title.str <- sprintf ("Predicted %s per instruction [%s]", response.var, arch)
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
