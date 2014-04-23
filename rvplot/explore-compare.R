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

prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL)
prompt.any.if.undef ("ALGS", keyword="algorithms", unique (Data[[ARCH]]$Algorithm))
prompt.any.if.undef ("CODES", keyword="implementations", unique (Data[[ARCH]]$Implementation))

assign.if.undef ("ANALYSIS.VARS", NULL)
assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

#======================================================================
# Analyze

D <- subset (Data[[ARCH]], Algorithm %in% ALGS & Implementation %in% CODES)
stopifnot (nrow (D) > 0) # Verify that D is non-empty
D$Time <- D$Time * 1e9 # Convert to nanoseconds
D$Implementation <- with (D, factor (Implementation, levels=rev (levels (Implementation))))

cat ("\nFirst few rows of the relevant data:\n")
print (head (D, n=12))
cat ("...\n\n")

# Compute the set of variables unique to this platform
Platform.vars <- setdiff (colnames (D), Common.vars)
Load.vars <- intersect (Platform.vars, All.load.vars)
Store.vars <- intersect (Platform.vars, All.store.vars)
Stall.vars <- intersect (Platform.vars, All.stall.vars)
has.cycles <- ("Cycles" %in% Platform.vars)

# Compute some per-instruction ratios
# Use 'Branch-based' instructions as the normalization factor
Index.vars <- c ("Algorithm", "Implementation", "Graph", "Iteration") # aggregation vars
Merge.vars <- setdiff (Index.vars, "Implementation")
D.inst.norm <- subset (D, Implementation == "Branch-based")[, c (Merge.vars, "Instructions")]
D.per.inst <- merge (D, D.inst.norm, by=Merge.vars, suffixes=c ("", ".norm"))
Agg.vars <- setdiff (colnames (D), Index.vars)
func.div.by.inst <- function (X, D.inst) return (colwise (function (X) X / D.inst$Instructions.norm) (X))
D.per.inst[, Agg.vars] <- func.div.by.inst (D.per.inst[, Agg.vars], D.inst=D.per.inst)

# Visualize fraction of instructions devoted to loads, stores,
# branches. Annotate with mispredictions.
FV <- split.df.by.colnames (D.per.inst, Index.vars)
F.per.inst <- flatten.keyvals.df (FV$A, FV$B)
Plot.vars <- c (Load.vars, Store.vars, "Branches", "Mispredictions")
Instructions.only <- subset (F.per.inst, Key %in% Plot.vars)
Instructions.only$Key <- with (Instructions.only, factor (Key, levels=Plot.vars))

Q <- ggplot (Instructions.only, aes (x=Graph, y=Value, colour=Key))
Q <- Q + geom_boxplot ()
Q <- Q + theme (legend.position="bottom")
Q <- Q + facet_grid (Algorithm ~ Implementation)
Q <- Q + theme (axis.text.x=element_text (angle=30, hjust=1), axis.ticks=element_blank ())
Q <- Q + scale_y_continuous (breaks=gen_ticks_linear (Instructions.only$Value, step=gen.stepsize.auto (Instructions.only$Value)$scaled), labels=percent)
Q <- Q + ylab ("")
Q <- Q + ggtitle ("Instruction mix, normalized by the Branch-based method")
Q <- set.hpcgarage.fill (Q)
Q <- set.hpcgarage.colours (Q)

if (!BATCH) {
  do.imix <- prompt.yes.no ("\nPlot instruction mix? ")
  if (do.imix) {
    setDevHD ()
    print (Q)
  }
  cat (sprintf ("\nSee also the 'instruction mix' plot (might be in a different window).\n"))
  pause.for.enter ()
}

stopifnot (FALSE)

# Define an initial list of variables to consider for analysis
Init.vars <- c (if (has.cycles) Platform.vars else "Time", "Branches", "Mispredictions")

# Don't consider any variable that is *all* zero
Is.all.zero <- colwise (function (X) all (X == 0)) (D.per.inst[, Init.vars])
Valid.vars <- names (Is.all.zero)[unlist (!Is.all.zero)]

# Compute numerical correlations
if (!BATCH) {
  do.cor <- prompt.yes.no ("\nPlot pairwise correlations? (may be slow) >>>")
  Cor.vars <- Valid.vars
  if (do.cor) {
    setDevSquare ()
    Q.cor <- ggpairs (D.per.inst, Cor.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor"))
    print (Q.cor)
  }
}

# Prompt user to select a subset of variables to further consider modeling
cat ("\n=== Correlations ===\n")
Rho <- cor (D.per.inst[, Cor.vars])
print (Rho)

if (!BATCH) {
  cat ("\n*** Inspect the above correlations and refer to the pairwise correlations plot. Decide which variables you'd like to consider.\n")
}

response.var <- if (has.cycles) "Cycles" else "Time" # Always consider
if (is.null (ANALYSIS.VARS)) {
  Avail.vars <- setdiff (Cor.vars, response.var)
  Analysis.vars <- prompt.select.any (Avail.vars, keyword="variables TO ELIMINATE")
  if (is.null (Analysis.vars)) {
    Analysis.vars <- Avail.vars
  }
} else {
  Analysis.vars <- ANALYSIS.VARS
}
cat ("\n==> Final set of analysis variables: ", Analysis.vars)
pause.for.enter (BATCH)

# Aggregate by Index.vars
D.max <- ddply (D, Index.vars, colwise (max))
D.tot <- ddply (D, Index.vars, colwise (function (X) sum (as.numeric (X))))
D.tot$Iteration <- D.max$Iteration + 1 # replace with count
D.tot.per.inst <- D.tot
D.tot.per.inst[, Agg.vars] <- func.div.by.inst (D.tot.per.inst[, Agg.vars], D.inst=D.tot)

# Try to fit the selected variables
#Data.fit <- D.per.inst[, c (Index.vars, response.var, Analysis.vars)]
Data.fit <- D.tot.per.inst[, c (Index.vars, response.var, Analysis.vars)]
Predictors <- Analysis.vars
Fit.nnlm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=FALSE, nonneg=TRUE)
print (summary (Fit.nnlm))

# Use the model to predict totals
Prediction <- predict.df.lm (Fit.nnlm, Data.fit, response.var)
response.true <- sprintf ("%s.true", response.var)
Prediction[, response.true] <- Data.fit[, response.var]
Prediction <- cbind (Data.fit[, Index.vars], Prediction)
print (head (Prediction))

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
Q.breakdown <- Q.breakdown + theme(axis.text.x=element_text(angle=35, hjust = 1))
Q.breakdown <- add.title.optsub (Q.breakdown, ggtitle, main=sprintf ("Predicted %s per instruction [%s / %s / %s]", response.var, ARCH, ALG, CODE))
Q.breakdown <- Q.breakdown + gen.axis.scale.auto (Y.values, "y")

Q.breakdown.display <- Q.breakdown
Q.breakdown.display <- Q.breakdown.display + theme (axis.text.x=element_text (size=10))
Q.breakdown.display <- Q.breakdown.display + theme (legend.text=element_text (size=14))
Q.breakdown.display <- Q.breakdown.display + theme (axis.text.y=element_text (size=10))
Q.breakdown.display <- Q.breakdown.display + theme (plot.title=element_text (size=20))

setDevHD ()
print (Q.breakdown)

cat (sprintf ("See model component breakdown plot.\n"))

outfilename <- sprintf ("figs2/explore-iters--cpi--%s--%s--%s.pdf"
                        , ARCH
                        , if (ALG == "SV") "sv" else "bfs"
                        , if (CODE == "Branch-based") "bb" else "bl")
cat (sprintf ("Output filename: %s [%s]\n", outfilename, if (SAVE.PDF) "saving..." else "*NOT* saving"))
if (SAVE.PDF) {
  Q.breakdown.pdf <- Q.breakdown
  Q.breakdown.pdf <- Q.breakdown.pdf + theme (axis.text.x=element_text (size=10))
  Q.breakdown.pdf <- Q.breakdown.pdf + theme (legend.text=element_text (size=14))
  Q.breakdown.pdf <- Q.breakdown.pdf + theme (axis.text.y=element_text (size=10))
  Q.breakdown.pdf <- Q.breakdown.pdf + theme (plot.title=element_text (size=20))
  setDevHD.pdf (outfilename, l=18)
  print (Q.breakdown.pdf)
  dev.off ()
}

# eof
