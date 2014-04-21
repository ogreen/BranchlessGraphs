#======================================================================
# Using this file to record commands used in some preliminary data analysis.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

Data <- load.perfdata.many ()
Common.vars <- get.common.colnames (Data)

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
FV <- split.df.by.colnames (D.tot.per.inst, Index.vars)
F.per.inst <- flatten.keyvals.df (FV$A, FV$B)
Plot.vars <- c ("Loads.Retired", "Stores.Retired", "Branches", "Mispredictions")
Instructions.only <- subset (F.per.inst, Key %in% Plot.vars)
Instructions.only$Key <- with (Instructions.only, factor (Key, levels=Plot.vars))
Q <- ggplot (Instructions.only, aes (x=Key, y=Value))
Q <- Q + geom_point (aes (colour=Key, shape=Key), size=4)
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
Cor.vars <- c ("Cycles", "Loads.Retired", "Stores.Retired", "Branches", "Mispredictions", "Stall.RS", "Stall.SB", "Stall.ROB")
setDevSquare ()
print (ggpairs (D.tot.per.inst, Cor.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor")))

# Fits a linear model. Note that since loads and branches are highly
# correlated, one is omitted.
Data.fit <- D.tot.per.inst # Data to fit
response.var <- "Cycles"
Predictors <- setdiff (Cor.vars, c (response.var, "Branches"))

Fit.lm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=FALSE)
print (summary (Fit.lm))

# Inspect the model's prediction, given the original predictors
Prediction <- predict.df.lm (Fit.lm, Data.fit, response.var)

# Add true measurement
response.true <- sprintf ("%s.true", response.var)
Prediction[, response.true] <- Data.fit[, response.var]
print (head (Prediction))

# Plot breakdown
Prediction <- cbind (Data.fit[, Index.vars], Prediction)
Prediction.FV <- split.df.by.colnames (Prediction, c (Index.vars, response.var, response.true))
Prediction.flat <- flatten.keyvals.df (Prediction.FV$A, Prediction.FV$B)

Y.values <- with (Prediction, c (Cycles, Cycles.true, Prediction.flat$Value))

Q.breakdown <- qplot (Graph, Value, data=Prediction.flat, geom="bar", stat="identity", fill=Key)
Q.breakdown <- Q.breakdown + theme (legend.position="bottom")
Q.breakdown <- Q.breakdown + xlab ("") + ylab ("") # Erase default labels
Q.breakdown <- set.hpcgarage.fill (Q.breakdown, name="Predicted values")
Q.breakdown <- Q.breakdown + geom_point (aes (x=Graph, y=Cycles), colour="black", fill=NA, data=Data.fit, shape=18, size=4) # Add measured values
Q.breakdown <- Q.breakdown + theme(axis.text.x=element_text(angle=35, hjust = 1))
Q.breakdown <- add.title.optsub (Q.breakdown, ggtitle, main=sprintf ("Predicted %s per instruction [%s / %s / %s]", response.var, ARCH, ALG, CODE))
Q.breakdown <- Q.breakdown + gen.axis.scale.auto (Y.values, "y")
setDevHD ()
print (Q.breakdown)

# Try alternative fit, forcing nonnegative coefficients
Fit.nnlm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=FALSE, nonneg=TRUE)
print (summary (Fit.nnlm))

# Inspect the model's prediction, given the original predictors
Prediction.nnlm <- predict.df.lm (Fit.nnlm, Data.fit, response.var)

# Add true measurement
response.true <- sprintf ("%s.true", response.var)
Prediction.nnlm[, response.true] <- Data.fit[, response.var]
print (head (Prediction.nnlm))

# Plot breakdown
Prediction.nnlm <- cbind (Data.fit[, Index.vars], Prediction.nnlm)
Prediction.nnlm.FV <- split.df.by.colnames (Prediction.nnlm, c (Index.vars, response.var, response.true))
Prediction.nnlm.flat <- flatten.keyvals.df (Prediction.nnlm.FV$A, Prediction.nnlm.FV$B)

Y.nnlm.values <- with (Prediction.nnlm, c (Cycles, Cycles.true, Prediction.nnlm.flat$Value))

Q.breakdown.nnlm <- qplot (Graph, Value, data=Prediction.nnlm.flat, geom="bar", stat="identity", fill=Key)
Q.breakdown.nnlm <- Q.breakdown.nnlm + theme (legend.position="bottom")
Q.breakdown.nnlm <- Q.breakdown.nnlm + xlab ("") + ylab ("") # Erase default labels
Q.breakdown.nnlm <- set.hpcgarage.fill (Q.breakdown.nnlm, name="Predicted values")
Q.breakdown.nnlm <- Q.breakdown.nnlm + geom_point (aes (x=Graph, y=Cycles), colour="black", fill=NA, data=Data.fit, shape=18, size=4) # Add measured values
Q.breakdown.nnlm <- Q.breakdown.nnlm + theme(axis.text.x=element_text(angle=35, hjust = 1))
Q.breakdown.nnlm <- add.title.optsub (Q.breakdown.nnlm, ggtitle, main=sprintf ("Predicted %s per instruction [%s / %s / %s]", response.var, ARCH, ALG, CODE))
Q.breakdown.nnlm <- Q.breakdown.nnlm + gen.axis.scale.auto (Y.nnlm.values, "y")
setDevHD ()
print (Q.breakdown.nnlm)

# eof
