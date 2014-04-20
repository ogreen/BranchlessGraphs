#======================================================================
# Using this file to record commands used in some preliminary data analysis.
#
# Each batch of commands is prefixed with the git revision and/or tag
# that was current when it was last tested.

source ("rvplot2-inc.R")
library (GGally)

Data <- load.perfdata.many ()

#======================================================================
# [rv bf3b4d7]

# Get (hsw, sv, branchy) data
D <- subset (Data[["hsw"]], Algorithm == "SV" & Implementation == "Branch-based")

# Aggregate over iterations
Index.vars <- c ("Algorithm", "Implementation", "Graph")

D.max <- ddply (D, Index.vars, colwise (max))
D.tot <- ddply (D, Index.vars, colwise (function (X) sum (as.numeric (X))))
D.tot$Iteration <- D.max$Iteration + 1 # replace iteration sum with count

# Compute some per-instruction ratios
Agg.vars <- setdiff (colnames (D), Index.vars)
D.per.inst <- D.tot
D.per.inst[, Agg.vars] <- colwise (function (X) X / D.tot$Instructions)(D.per.inst[, Agg.vars])

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
