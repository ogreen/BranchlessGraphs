#======================================================================
# Rich's "working" script for recording a few ad-hoc analyses

rm (list=ls ())
source ("rvplot2-inc.R")

#======================================================================
ARCHS <- as.vector (ARCHS.ALL.MAP)
assign.if.undef ("All.data", load.perfdata.many ())
CODES <- unique (get.all.colvals (All.data, "Implementation"))
ALGS <- unique (get.all.colvals (All.data, "Algorithm"))

# *All* the data
Df <- get.perfdf (All.data, ARCHS, ALGS, CODES)
Vars <- get.perfdf.var.info (Df, All.data)

# Totals
Df.tot <- total.perfdf (Df, Vars)

# What is the minimum number of iterations for any graph?
Iterations.per.graph <- ddply (Df.tot, .(Graph), summarise, Min=min (Iteration))

assign.if.undef ("MIN.ITERS", 5)
Keepers <- (Iterations.per.graph$Min >= MIN.ITERS)
GRAPHS <- Iterations.per.graph$Graph[Keepers]

cat (sprintf ("# These graphs have at least %d iterations for any (algorithm, implementation):\n", MIN.ITERS))
cat (sprintf ("GRAPHS <- c (\"%s\")\n", paste (GRAPHS, collapse="\", \"")))

stopifnot (FALSE)
#======================================================================

# Analysis: Compare different fitting procedures

BATCH <- TRUE
SAVE.PDF <- FALSE
ARCH <- "Haswell"
#ALGS <- "SV"
#CODES <- "Branch-based"
#GRAPHS <- "power"

#FIT.PER.GRAPH <- TRUE
source ("explore-cpi.R")

# Experiments: Other fittings
Df.fit <- subset (Df.per.inst, Algorithm == "SV" & Implementation == "Branch-based")
Possible.predictors <- setdiff (Vars$Predictors, "Instructions")
Cols.predict <- c (Vars$Index, response.var, Possible.predictors)
Df.predict <- subset (Df.tot.per.inst[, Cols.predict], Algorithm == "SV" & Implementation == "Branch-based")

if (TRUE) {
  source ("fit-cpi-lasso-inc.R")

  Fit <- fit.lm.lasso (response.var, Possible.predictors, Df.fit, const.term=CONST.TERM, nonneg=NONNEG)
  Df.predict <- predict.lm.lasso (Fit, Df.predict)

  cat ("=== Lasso-fitted coefficients ===\n")
  Coefs.nz <- coefs.nz.lm.lasso (Fit)
  Coefs.zero <- setdiff (Possible.predictors, names (coefs.nz.lm.lasso (Fit)))
  print (Coefs.nz)
  cat (sprintf ("(%s are all zero)\n\n", paste (Coefs.zero, collapse=", ")))
} else {
  # try glmnet
  library (glmnet)

  y <- Df.fit[[response.var]]
  X <- as.matrix (Df.fit[, Possible.predictors])
  fit <- cv.glmnet (X, y, lower.limits=0, intercept=FALSE)
  #fit <- cv.glmnet (X, y, lower.limits=0, intercept=FALSE, alpha=0.2) # intercept=FALSE doesn't give good results?

  lambda <- fit$lambda.min
  Coefs.all <- coef (fit, s=lambda)
  Coefs.all.names <- rownames (Coefs.all)
  K.nz <- summary (Coefs.all)$i # Non-zero coefficients
  Coefs.nz.names <- setdiff (Coefs.all.names[K.nz], "(Intercept)") # Excludes "(Intercept)"
  Alpha <- as.vector (Coefs.all)

  X.predict <- as.matrix (Df.predict[, Possible.predictors])
  stopifnot (all (colnames (X) == colnames (X.predict)))

  Y.predict <- X.predict %*% diag (Alpha[2:length (Alpha)])
  Y.predict[, 1] <- Y.predict[, 1] + Coefs.all["(Intercept)", 1]
  y.predict <- predict (fit, newx=X.predict, s="lambda.min")
  colnames (y.predict) <- response.var
  Df.predict[, Possible.predictors] <- Y.predict
  Df.predict[, response.true] <- Df.predict[, response.var]
  Df.predict[, response.var] <- y.predict
}

Fixed.cols <- c (Vars$Index, response.var, response.true)
Df.predict.FV <- split.df.by.colnames (Df.predict, Fixed.cols)
Df.predict.flat <- flatten.keyvals.df (Df.predict.FV$A, Df.predict.FV$B)

Y.predicted <- Df.predict[[response.var]]
Y.true <- Df.predict[[response.true]]
Other.values <- Df.predict.flat$Value
Y.values <- with (Df.predict, c (Y.predicted, Y.true, Other.values))

if (FACET.COL == "Implementation") {
  Df.predict.flat$X <- Df.predict.flat$Graph
  Df.predict.flat$Group <- Df.predict.flat$Implementation
  Df.predict$X <- Df.predict$Graph
  Df.predict$Group <- Df.predict$Implementation
} else {
  stopifnot (FACET.COL == "Graph")
  Df.predict.flat$X <- Df.predict.flat$Implementation
  Df.predict$X <- Df.predict$Implementation
  Df.predict.flat$Group <- Df.predict.flat$Graph
  Df.predict$Group <- Df.predict$Graph
}  

Q.cpi.altfit <- qplot (X, Value, data=Df.predict.flat, geom="bar", stat="identity", fill=Key)
Q.cpi.altfit <- Q.cpi.altfit + geom_point (aes (x=X, y=Cycles.true), data=Df.predict
                             , colour="grey", fill=NA, shape=18, size=4)
Q.cpi.altfit <- Q.cpi.altfit + facet_grid (Algorithm ~ Group, scales="free_y")

Q.cpi.altfit <- Q.cpi.altfit + theme(axis.text.x=element_text(angle=35, hjust = 1))

Q.cpi.altfit <- set.hpcgarage.fill (Q.cpi.altfit, name="Predicted values: ")
Q.cpi.altfit <- Q.cpi.altfit + theme (legend.position="bottom")
Q.cpi.altfit <- Q.cpi.altfit + xlab ("") + ylab ("") # Erase default labels

title.str <- sprintf ("Predicted %s per instruction [%s]", response.var, ARCH)
Q.cpi.altfit <- add.title.optsub (Q.cpi.altfit, ggtitle, main=title.str)
#Q.cpi.altfit <- Q.cpi.altfit + gen.axis.scale.auto (Y.values, "y")

Q.cpi.altfit.display <- set.all.font.sizes (Q.cpi.altfit, base=10)

stopifnot (FALSE)
#======================================================================

F.lm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=CONST.TERM, nonneg=FALSE)
YY <- Data.fit
YY$Measured <- YY[[response.var]]
YY$Modeled <- F.lm$fitted.values
qplot (Measured, Modeled-Measured, data=YY) + facet_wrap (~ Graph) + geom_hline (yintercept=0)

F.nnlm <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=CONST.TERM, nonneg=TRUE)

# lm diagnostics
setDevHD () ; par (mfrow=c (2, 2)) ; plot (F.lm)
library (gvlma) ; gvmodel <- gvlma (F.lm) ; summary (gvmodel)

# Sum of squares for nnlm model
#Y.true <- as.vector (unlist (Data.fit[F.nnlm$response.var]))
#Y.fit <- Y.true + as.vector (unlist (F.nnlm$model$residuals))
#summary (lm (Y.fit ~ Y.true - 1))

CONST.TERM <- TRUE
source ("explore-cpi.R")
F.lm.c <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=CONST.TERM, nonneg=FALSE)
F.nnlm.c <- lm.by.colnames (Data.fit, response.var, Predictors, constant.term=CONST.TERM, nonneg=TRUE)

# eof
