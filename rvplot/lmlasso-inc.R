#======================================================================
# This module implements a fitting procedure based on lasso
# regression, as implemented in the 'penalized' package:
#
# http://cran.r-project.org/web/packages/penalized/vignettes/penalized.pdf

library (penalized)

is.lm.lasso <- function (obj) {
  return ("lm.lasso" %in% class (obj))
}

get.lm.formula.string <- function (response.var, Predictors, intercept) {
  intercept.str <- if (!intercept) "0 + " else ""
  preds.str <- paste (Predictors, collapse=" + ")
  return (sprintf ("%s ~ %s%s", response.var, intercept.str, preds.str))
}

fit.lm.lasso <- function (response.var, Possible.predictors, Df.fit
                          , const.term=FALSE, nonneg=TRUE, verbose=TRUE)
{
  stopifnot (is.character (response.var))
  stopifnot (is.character (Possible.predictors))
  stopifnot (is.data.frame (Df.fit))

  if (verbose) {
    cat (sprintf ("Lasso fitting: %s ...\n"
                  , get.lm.formula.string (response.var
                                           , Possible.predictors
                                           , intercept=const.term)))
  }

  y <- Df.fit[[response.var]]
  X <- Df.fit[, Possible.predictors]
  if (const.term) {
    fit <- penalized (y, X, data=Df.fit, positive=nonneg)
  } else {
    fit <- penalized (y, X, unpenalized=~0, data=Df.fit, positive=nonneg)
  }

  # Create lm.lasso object
  Fit <- list ()
  Fit$fit <- fit
  Fit$response.var <- response.var
  Fit$Possible.predictors <- Possible.predictors
  Fit$const.term <- const.term
  Fit$nonneg <- nonneg
  class (Fit) <- c ("lm.lasso", class (Fit))
  return (Fit)
}

# Returns just the non-zero coefficients
coefs.nz.lm.lasso <- function (Fit) {
  stopifnot (is.lm.lasso (Fit))
  return (coefficients (Fit$fit))
}

# Returns all coefficients, with zeroes set explicitly
coefs.lm.lasso  <- function (Fit) {
  stopifnot (is.lm.lasso (Fit))

  Possible.predictors <- Fit$Possible.predictors
  Coefs.nz <- coefs.nz.lm.lasso (Fit) # possibly sparse
  
  # Expand to dense
  Coefs.all <- rep (0, length (Possible.predictors))
  names (Coefs.all) <- Possible.predictors
  Coefs.all[names (Coefs.nz)] <- Coefs.nz
  return (Coefs.all)
}

# Predict, given a fit and new data
predict.lm.lasso <- function (Fit, Df.predict
                              , response.true=NULL
                              , return.frame=TRUE
                              , all.components=TRUE) {
  stopifnot (is.lm.lasso (Fit))

  fit <- Fit$fit
  Possible.predictors <- Fit$Possible.predictors
  response.var <- Fit$response.var

  p <- predict (fit, Df.predict[, Possible.predictors])
  if (is.matrix (p)) {
    m <- p
  } else {
    m <- t (as.matrix (p))
  }
  y.predict <- m[, "mu"]
  if (is.numeric (y.predict)) {
    names (y.predict) <- response.var
  } else {
    colnames (y.predict) <- response.var
  }
  if (!all.components) { return (y.predict) }

  Alpha <- coefs.lm.lasso (Fit)
  X.predict <- as.matrix (Df.predict[, Possible.predictors])
  Y.predict <- X.predict %*% diag (Alpha)
  colnames (Y.predict) <- Possible.predictors

  if (!return.frame) {
    return (cbind (y.predict, Y.predict))
  }
  
  if (is.null (response.true)) {
    response.true <- sprintf ("%s.true", response.var)
  }
  Df.predict[, Possible.predictors] <- Y.predict
  Df.predict[, response.true] <- Df.predict[, response.var]
  Df.predict[, response.var] <- y.predict
  return (Df.predict)
}

# eof
