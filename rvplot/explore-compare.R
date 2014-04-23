#======================================================================
# Using this file to record commands used in some preliminary data analysis.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

Data <- load.perfdata.many ()
Common.vars <- get.common.colnames (Data)
All.vars <- get.all.colnames (Data)

# Pull out some special variables
All.load.vars <- All.vars[grep ("^Loads.*", All.vars)]
All.store.vars <- All.vars[grep ("^Stores.*", All.vars)]
All.stall.vars <- All.vars[grep ("^Stalls.*", All.vars)]

All.codes <- unique (get.all.colvals (Data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL.MAP)
prompt.any.if.undef ("ALGS", keyword="algorithms", unique (Data[[ARCH]]$Algorithm))
prompt.any.if.undef ("CODES", keyword="implementations", unique (Data[[ARCH]]$Implementation))

assign.if.undef ("ANALYSIS.VARS", NULL)
assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALGS, CODES)
outfilename.cpi <- sprintf ("figs2/explore-compare--cpi--%s.pdf", outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  CPI: %s\n", outfilename.cpi))

#======================================================================
# Analyze

D <- get.perfdf.arch (Data, ARCH, ALGS, CODES)


# Define an initial list of variables to consider for analysis
Init.vars <- c (if (has.cycles) Platform.vars else "Time", "Branches", "Mispredictions")

# Don't consider any variable that is *all* zero
Is.all.zero <- colwise (function (X) all (X == 0)) (D.per.inst[, Init.vars])
Valid.vars <- names (Is.all.zero)[unlist (!Is.all.zero)]

# Compute numerical correlations
if (!BATCH) {
  do.cor <- prompt.yes.no ("\nPlot pairwise correlations? (may be slow) >>> ")
  if (do.cor) {
    setDevSquare ()
    Q.cor <- ggpairs (D.per.inst, Valid.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor"))
    print (Q.cor)
  }
}

# Prompt user to select a subset of variables to further consider modeling
cat ("\n=== Correlations ===\n")
Rho <- cor (D.per.inst[, Valid.vars])
print (Rho)

if (!BATCH) {
  cat ("\n*** Inspect the above correlations and refer to the pairwise correlations plot. Decide which variables you'd like to consider.\n")
}

response.var <- if (has.cycles) "Cycles" else "Time" # Always consider
if (is.null (ANALYSIS.VARS)) {
  Avail.vars <- setdiff (Valid.vars, response.var)
  Analysis.vars <- prompt.select.any (Avail.vars, keyword="variables TO ELIMINATE")
  if (is.null (Analysis.vars)) {
    Analysis.vars <- Avail.vars
  }
} else {
  Analysis.vars <- ANALYSIS.VARS
}
cat ("\n==> Final set of analysis variables: ", Analysis.vars)
pause.for.enter (BATCH)

# Aggregate over iterations
Select.vars <- setdiff (Index.vars, "Iteration")
D.max <- ddply (D, Select.vars, colwise (max))
D.tot <- ddply (D, Select.vars, colwise (function (X) sum (as.numeric (X))))
D.tot$Iteration <- D.max$Iteration + 1 # replace with count
D.tot$Instructions.norm <- D.tot$Instructions # Needed for 'func.div.by.inst'
D.tot.per.inst <- D.tot
D.tot.per.inst[, Agg.vars] <- func.div.by.inst (D.tot.per.inst[, Agg.vars], D.inst=D.tot)

# Compute one fit per code & algorithm
Predictions <- NULL
for (alg in ALGS) {
  for (code in CODES) {
    cat (sprintf ("=== Fitting: %s & %s ===\n", alg, code))
    
    # Perform a fit
    Data.fit <- subset (D.tot.per.inst[, c (Select.vars, response.var, Analysis.vars)]
                        , Algorithm == alg & Implementation == code)
    Predictors <- Analysis.vars
    Fit.nnlm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=FALSE, nonneg=TRUE)
    print (summary (Fit.nnlm))

    # Use the model to predict totals
    Prediction <- predict.df.lm (Fit.nnlm, Data.fit, response.var)
    response.true <- sprintf ("%s.true", response.var)
    Prediction[, response.true] <- Data.fit[, response.var]
    Prediction <- cbind (Data.fit[, Select.vars], Prediction)
    print (head (Prediction))

    # Save
    Predictions <- rbind (Predictions, Prediction)
  }
}

# Plot breakdown
Prediction.FV <- split.df.by.colnames (Prediction, c (Index.vars, response.var, response.true))
Prediction.flat <- flatten.keyvals.df (Prediction.FV$A, Prediction.FV$B)

Y.true <- Prediction[[response.true]]
Y.values <- with (Prediction, c (Prediction[[response.var]], Y.true, Prediction.flat$Value))

Q.breakdown <- qplot (Graph, Value, data=Prediction.flat, geom="bar", stat="identity", fill=Key)
Q.breakdown <- Q.breakdown + theme (legend.position="bottom")
Q.breakdown <- Q.breakdown + xlab ("") + ylab ("") # Erase default labels
Q.breakdown <- set.hpcgarage.fill (Q.breakdown, name="Predicted values: ")
Q.breakdown <- Q.breakdown + geom_point (aes (x=Graph, y=Y.true), colour="black", fill=NA, data=Data.fit, shape=18, size=4) # Add measured values
Q.breakdown <- Q.breakdown + theme (axis.text.x=element_text(angle=30, hjust = 1))
Q.breakdown <- add.title.optsub (Q.breakdown, ggtitle, main=sprintf ("Predicted %s per instruction [%s / %s / %s]", response.var, ARCH, ALG, CODE))
Q.breakdown <- Q.breakdown + gen.axis.scale.auto (Y.values, "y")

setDevHD ()
Q.breakdown.display <- set.all.font.sizes (Q.breakdown, base=10)
print (Q.breakdown.display)

cat (sprintf ("See model component breakdown plot.\n"))

if (SAVE.PDF) {
  Q.breakdown.pdf <- set.all.font.sizes (Q.breakdown, base=12)
  setDevHD.pdf (outfilename.cpi, l=18)
  print (Q.breakdown.pdf)
  dev.off ()
}

# eof
