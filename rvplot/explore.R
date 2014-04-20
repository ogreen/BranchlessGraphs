#======================================================================
# Using this file to record commands used in some preliminary data analysis.

source ("rvplot2-inc.R")
library (GGally)

Data <- load.perfdata.many ()
Common.vars <- get.common.colnames (Data)

flatten.df <- function (Df, Fixed.cols, Var.cols) {
  stopifnot (is.data.frame (Df))
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

# Get (hsw, sv, branchy) data
D <- subset (Data[["hsw"]], Algorithm == "SV" & Implementation == "Branch-based")
head (D)

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
D.per.inst <- D.tot
D.per.inst[, Agg.vars] <- colwise (function (X) X / D.tot$Instructions)(D.per.inst[, Agg.vars])

# Visualize fraction of instructions devoted to loads, stores, and branches
F.per.inst <- flatten.df (Df=D.per.inst, Fixed.cols=Index.vars, Var.cols=setdiff (colnames (D.per.inst), Index.vars))
Instructions.only <- subset (F.per.inst, Var %in% c ("Branches", "Loads.Retired", "Stores.Retired", "Mispredictions"))
Instructions.only$Var <- with (Instructions.only, factor (Var, levels=rev (c ("Loads.Retired", "Stores.Retired", "Branches", "Mispredictions"))))
Instructions.only$Value <- with (Instructions.only, ifelse (Var == "Mispredictions", 0, Value))
Mispredictions.only <- subset (F.per.inst, Var == "Mispredictions")
Q <- qplot (Graph, Value, data=Instructions.only, geom="bar", stat="identity", fill=Var, colour=Var)
Q <- Q + geom_point (aes (y=Value, colour=Var), data=Mispredictions.only, size=5)
Q <- Q + ylab ("% instructions")
Q <- set.hpcgarage.fill (Q)
Q <- set.hpcgarage.colours (Q)
print (Q)

# Fit CPI to loads, stores, branches per instruction
L <- with (D.per.inst, lm (Cycles ~ Loads.Retired + Stores.Retired + Branches + Mispredictions))
print (summary (L))

setDevSquare ()
Cor.vars <- c ("Cycles", "Loads.Retired", "Stores.Retired", "Branches", "Mispredictions")
print (ggpairs (D.per.inst, Cor.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor")))

# Loads and branches are highly correlated; drop one
L.2 <- with (D.per.inst, lm (Cycles ~ Loads.Retired + Stores.Retired + Mispredictions))
print (summary (L.2))

# eof
