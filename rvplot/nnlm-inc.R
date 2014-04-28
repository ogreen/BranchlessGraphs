#=====================================================================
# This module implements an object wrapper around a linear regression
# model, whether it uses the standard model or a nonnegative model
# (e.g., via nnls).

library (nnls)

#=====================================================================
# Wrapper around 'lm' to perform a regression fit, given a data frame
# and the response and predictor variables given by column
# names. (That is, by column names instead of by a formula, as in
# 'lm'.)

lm.by.colnames <- function (Data.fit, response.var, Predictors
                            , const.term=TRUE, nonneg=FALSE
                            , verbose=TRUE)
{
  Fit.m <- NULL

  # Attempt the standard 'lm'
  lhs <- response.var
  rhs.const <- if (const.term) "" else "0 + "
  rhs.preds <- paste (Predictors, collapse=" + ")
  fit.formula <- as.formula (sprintf ("%s ~ %s%s", lhs, rhs.const, rhs.preds))
  if (verbose) {
    cat ("==> Fitting formula: ")
    print (fit.formula)
    cat ("\n")
  }
  Fit.lm <- lm (fit.formula, data=Data.fit)

  any.problems <- any (is.na (Fit.lm$coef))
  any.neg <- any (Fit.lm$coef[!is.na (Fit.lm$coef)] < 0)
  
  if (any.problems | (nonneg & any.neg)) { # user wants non-negative coefficients
    X <- as.matrix (Data.fit[, Predictors])
    if (const.term) {
      X <- cbind (1, X)
    }
    y <- as.vector (unlist (Data.fit[response.var]))
    Fit.lm <- list (model=nnls (X, y)
                    , constant.term=const.term
                    , response.var=response.var
                    , Predictors=Predictors
                    , lm=Fit.lm
                    , any.problems=any.problems
                    , any.neg=any.neg)
    class (Fit.lm) <- "nnlm"
  }
  
  # Compute residual stats
  Prediction <- predict.df.lm (Fit.lm, Data.fit, response.var=response.var)
  Y.obs <- Data.fit[[response.var]]
  Y.pred <- Prediction[[response.var]]
  Fit.lm$mu.obs <- mean (Y.obs)
  Fit.lm$ss.tot <- sum ((Y.obs - Fit.lm$mu.obs)^2)
  Fit.lm$ss.res <- sum ((Y.pred - Y.obs)^2)
  Fit.lm$res2 <- max (1 - (Fit.lm$ss.res / Fit.lm$ss.tot), 0)
  Fit.lm$y.obs <- Y.obs
  Fit.lm$y.pred <- Y.pred

  return (Fit.lm)
}

# Given an 'lm' object, return the intercept value
get.intercept.lm <- function (fit.lm) {
  stopifnot (any (class (fit.lm) %in% c ("lm", "nnlm")))

  fit.intercept <- NULL
  if (any (class (fit.lm) == "nnlm")) {
    if (fit.lm$constant.term) {
      fit.intercept <- fit.lm$model$x[1]
    }
  } else { # any (class (fit.lm) == "lm")
    if ("(Intercept)" %in% names (fit.lm$coef)) {
      fit.intercept <- fit.lm$coef[["(Intercept)"]]
    }
  }
  return (fit.intercept)
}

# Given an 'lm' object, return the predictor weights (coefficients)
get.predictor.coefs.lm <- function (fit.lm) {
  stopifnot (any (class (fit.lm) %in% c ("lm", "nnlm")))

  if (any (class (fit.lm) == "nnlm")) {
    Fit.coefs <- fit.lm$model$x
    if (fit.lm$constant.term) {
      Fit.coefs <- Fit.coefs[2:length (Fit.coefs)] # 1st term is intercept
    }
    names (Fit.coefs) <- fit.lm$Predictors
  } else { # any (class (fit.lm) == "lm")
    Predictors <- setdiff (names (fit.lm$coef), "(Intercept)")
    Fit.coefs <- fit.lm$coef[Predictors]
  }
  return (Fit.coefs)
}

# Given an 'lm' object L and a set of predictor values, given as
# columns of a data frame D, this function makes predictions. It
# returns a new data frame with a column for the intercept term (or
# "0" if the model does not have an intercept), a column for each
# predictor term, and a final column with the overall prediction
# (i.e., sum of the other columns).
#
# Note: The data frame must have columns whose names exactly match the
# names of the 'lm' predictors.
#
predict.df.lm <- function (fit.lm, Df, response.var="Y") {
  stopifnot (any (class (fit.lm) %in% c ("lm", "nnlm")))
  stopifnot (is.data.frame (Df))
  
  # Extract fit coefficients for later use. If an intercept is
  # available, extract it as a separate variable (or NULL if not available).
  fit.intercept <- get.intercept.lm (fit.lm)
  Fit.coefs <- get.predictor.coefs.lm (fit.lm)

  stopifnot (all (names (Fit.coefs) %in% colnames (Df)))

  # Insert a column for the intercept term, or 0 if not available
  if (is.null (fit.intercept)) {
    Prediction <- NULL
  } else {
    Prediction <- data.frame (B0=rep (fit.intercept, nrow (Df)))
    Prediction <- rename.col (Prediction, old="B0", new="(Intercept)")
  }

  # Insert a column for each predictor
  for (c in names (Fit.coefs)) {
    # Compute new column
    X.c <- Fit.coefs[[c]] * Df[[c]]
    if (is.null (Prediction)) { # Initialize
      Prediction <- rename.col (data.frame (X.c), old="X.c", new=c)
    } else { # Add column
      Prediction[c] <- Fit.coefs[[c]] * Df[[c]]
    }
  }

  # Lastly, insert a column for the final prediction
  Prediction[, response.var] <- apply (Prediction, 1, sum)
  return (Prediction)
}

summary.nnlm <- function (X) {
  print (X)
}

#=====================================================================
# eof
