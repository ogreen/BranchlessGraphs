library (penalized)

is.lm.lasso <- function (obj) {
  return ("lm.lasso" %in% class (obj))
}

get.lm.formula.string <- function (response.var, Predictors, intercept) {
  return (sprintf ("%s ~ %s%s"
                   , response.var
                   , if (!intercept) "0 + " else ""
                  , paste (Possible.predictors, collapse=" + ")))
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
                              , all.components=FALSE) {
  stopifnot (is.lm.lasso (Fit))

  Predictors.all <- Fit$Predictors.all
  response.var <- Fit$response.var
  
  y.predict <- as.matrix (predict (fit, Df.predict[, Predictors.all])[, "mu"])
  colnames (y.predict) <- response.var
  if (!all.components) { return (y.predict) }

  X.predict <- as.matrix (Df.predict[, Predictors.all])
  Y.predict <- X.predict %*% diag (Alpha)
  colnames (Y.predict) <- Predictors.all

  if (!return.frame) {
    return (cbind (y.predict, Y.predict))
  }
  
  if (is.null (response.true)) {
    response.true <- sprintf ("%s.true", response.var)
  }
  Df.predict[, Predictors.all] <- Y.predict
  Df.predict[, response.true] <- Df.predict[, response.var]
  Df.predict[, response.var] <- y.predict
  return (Df.predict)
}

# eof
