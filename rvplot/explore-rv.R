# Rich's "working" script for recording a few ad-hoc analyses

# Analysis: Compare different fitting procedures
rm (list=ls ())
source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- FALSE
ARCH <- "Haswell"
#ALGS <- "SV"
#CODES <- "Branch-based"
#GRAPHS <- "power"

FIT.PER.GRAPH <- TRUE
source ("explore-cpi.R")

stopifnot (FALSE)

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
