#======================================================================
# Using this file to record commands used in some preliminary data analysis.

source ("rvplot2-inc.R")
library (GGally)

Data <- load.perfdata.many ()
Common.vars <- get.common.colnames (Data)

flatten.df <- function (Df, Fixed.cols, Var.cols=NULL) {
  stopifnot (is.data.frame (Df))

  # If Var.cols not specified, fill in default
  if (is.null (Var.cols)) {
    Var.cols <- setdiff (names (Df), Fixed.cols)
  }
  
  New.df <- NULL
  Fixed.df <- Df[, Fixed.cols]
  for (Var in Var.cols) {
    Value <- Df[[Var]]
    New.df <- rbind (New.df, cbind (Fixed.df, Var, Value))
  }
  return (New.df)
}

merge.df.list <- function (Df.list, on.cols=NULL) {
  if (is.data.frame (Df.list)) return (Df.list)
  stopifnot (is.list (Df.list))

  Common.cols <- if (is.null (on.cols)) get.common.cols (Df.list) else on.cols
  Merged.df <- NULL
  for (df.name in names (Df.list)) {
    Merged.df <- rbind (Merged.df, Df.list[[df.name]][, Common.cols])
  }
  return (Merged.df)
}

#======================================================================
# This first analysis considers just (HSW, SV, Branchy), and focuses
# on *total* operations, i.e., summed over all iterations for each
# problem.
#
# Some observations:
# - Loads and branches are highly correlated, which makes sense since
#   every branch is preceded by a load.
# - Loads and stores are anti-correlated. (Why?)
# - Cycles and mispredictions are most strongly correlated. (Why?)
# - Cycles and RS stalls are not correlated. (Why not?)
# - Linear regression with an intercept produces a large, negative
#   intercept -- so not useful / hard to interpret
# - Linear regression without an intercept produces a better fit than
#   above, but with a small negative "stores" coefficient -- so also -
#   hard to interpret. Nevertheless, it gives a nice result that
#   attributes most of the execution time variability to branch
#   mispredictions, as we'd hope.
#
#
# Questions / issues:
#
# - If cycles and mispredictions are strongly correlated, but RS
#   stalls are not, what may we conclude?
# - Need to try an 'nnls' (nonnegative least squares) fit instead
# - We are not including cache misses in our counters. We may need to
# - include them, so that we can justify excluding them. :)

# Get (hsw, sv, branchy) data
ARCH <- "hsw"
ALG <- "SV"
CODE <- "Branch-based"

D <- subset (Data[[ARCH]], Algorithm == ALG & Implementation == CODE)
stopifnot (nrow (D) > 0) # Verify that D is non-empty
print (head (D))

# Compute the set of variables unique to this platform
Platform.vars <- setdiff (colnames (D), Common.vars)

# Computes 'max' and 'totals' for each variable
# Note: This is the sum for all variables *except* 'Iteration'
Index.vars <- c ("Algorithm", "Implementation", "Graph") # aggregation vars
D.max <- ddply (D, Index.vars, colwise (max))
D.tot <- ddply (D, Index.vars, colwise (function (X) sum (as.numeric (X))))
D.tot$Iteration <- D.max$Iteration + 1 # replace with count

# Compute some per-instruction ratios
Agg.vars <- setdiff (colnames (D), Index.vars)
D.tot.per.inst <- D.tot
D.tot.per.inst[, Agg.vars] <- colwise (function (X) X / D.tot$Instructions)(D.tot.per.inst[, Agg.vars])

# Visualize fraction of instructions devoted to loads, stores, and branches
F.per.inst <- flatten.df (Df=D.tot.per.inst, Fixed.cols=Index.vars, Var.cols=setdiff (colnames (D.tot.per.inst), Index.vars))
Plot.vars <- c ("Loads.Retired", "Stores.Retired", "Branches", "Mispredictions")
Instructions.only <- subset (F.per.inst, Var %in% Plot.vars)
Instructions.only$Var <- with (Instructions.only, factor (Var, levels=Plot.vars))
Q <- ggplot (Instructions.only, aes (x=Var, y=Value))
Q <- Q + geom_point (aes (colour=Var, shape=Var), size=4)
Q <- Q + theme (legend.position="bottom")
Q <- Q + facet_grid (. ~ Graph)
Q <- Q + theme (axis.text.x=element_blank (), axis.ticks=element_blank ())
Q <- Q + scale_y_continuous (breaks=gen_ticks_linear (Instructions.only$Value, step=gen.stepsize.auto (Instructions.only$Value)$scaled), labels=percent)
Q <- Q + ylab ("% instructions")
Q <- set.hpcgarage.fill (Q)
Q <- set.hpcgarage.colours (Q)
setDevSlide ()
print (Q)

# Visualize correlations
Cor.vars <- c ("Cycles", "Loads.Retired", "Stores.Retired", "Branches", "Mispredictions", "Stall.RS")
setDevSquare ()
print (ggpairs (D.tot.per.inst, Cor.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor")))

# Fits a linear model. Note that since loads and branches are highly
# correlated, one is omitted.
Data.fit <- D.tot.per.inst # Data to fit
response.var <- "Cycles"
Predictors <- setdiff (Cor.vars, c (response.var, "Branches"))

fit.formula <- as.formula (paste (c (response.var, paste (Predictors, collapse=" + ")), collapse=" ~ 0 + "))
cat ("==> Fitting formula: ") ; print (fit.formula) ; cat ("\n")
Fit.lm <- lm (fit.formula, data=Data.fit)
print (summary (Fit.lm))

# Extract fit coefficients for later use. If an intercept is
# available, extract it as a separate variable (or NULL if not available).
if ("(Intercept)" %in% names (Fit.lm$coef)) {
  fit.intercept <- Fit.lm$coef[["(Intercept)"]]
} else {
  fit.intercept <- NULL
}
Fit.coefs <- Fit.lm$coef[setdiff (names (Fit.lm$coef), "(Intercept)")]

# Inspect the model's prediction, given the original predictors
if (is.null (fit.intercept)) {
  Prediction <- NULL
} else {
  Prediction <- data.frame (B0=rep (fit.intercept, nrow (Data.fit)))
  Prediction <- rename.col (Prediction, old="B0", new="(Intercept)")
}
for (c in names (Fit.coefs)) {
  # Compute new column
  X.c <- Fit.coefs[[c]] * Data.fit[[c]]
  if (is.null (Prediction)) { # Initialize
    Prediction <- rename.col (data.frame (X.c), old="X.c", new=c)
  } else { # Add column
    Prediction[c] <- Fit.coefs[[c]] * Data.fit[[c]]
  }
}
Prediction[, response.var] <- apply (Prediction, 1, sum)

# Add 
response.true <- sprintf ("%s.true", response.var)
Prediction[, response.true] <- Data.fit[, response.var]
head (Prediction)

# Plot breakdown
Prediction <- cbind (Data.fit[, Index.vars], Prediction)
Prediction.flat <- flatten.df (Prediction, Fixed.cols=c (Index.vars, response.var, response.true))

Q.breakdown <- qplot (Graph, Value, data=Prediction.flat, geom="bar", stat="identity", fill=Var)
Q.breakdown <- Q.breakdown + theme (legend.position="bottom")
Q.breakdown <- Q.breakdown + xlab ("") + ylab ("") # Erase default labels
Q.breakdown <- set.hpcgarage.fill (Q.breakdown, name="Predicted values")
Q.breakdown <- Q.breakdown + geom_point (aes (x=Graph, y=Cycles), colour="black", fill=NA, data=Data.fit, shape=18, size=4) # Add measured values
Q.breakdown <- Q.breakdown + theme(axis.text.x=element_text(angle=35, hjust = 1))
Q.breakdown <- add.title.optsub (Q.breakdown, ggtitle, main=sprintf ("Predicted %s per instruction [%s / %s / %s]", response.var, ARCH, ALG, CODE))
Q.breakdown <- Q.breakdown + gen.axis.scale.auto (Prediction.flat$Value, "y")
setDevHD ()
print (Q.breakdown)

# eof
