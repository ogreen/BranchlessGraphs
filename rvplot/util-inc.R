#=====================================================================
# Assigns a variable a given value, but *only* if the variable doesn't
# already exist; otherwise, leaves the variable's value intact.

assign.if.undef <- function (var.name, value.if.new, env=parent.frame (), ...)
  if (!(var.name %in% ls (envir=env))) assign (var.name, value.if.new, envir=env, ...)

#=====================================================================
# Error handling

# Halt if the condition is true
stopif <- function (cond) stopifnot (!cond)

# If 'cond' is true, prints 'msg' to the console and then either stops
# the program ('fatal=TRUE') or displays a warning.
check.cond <- function (cond, msg, fatal=FALSE) {
  if (!cond) {
    if (fatal) {
      stop (msg)
    } else {
      warning (msg)
    }
  }
  return (cond)
}

#=====================================================================
# Display a sampling of data frame rows

get.sample <- function (DF, n=10, silent=TRUE) {
  stopifnot (is.data.frame (DF))
  S <- sample (rownames (DF), n)
  DF.sample <- DF[S,]
  if (!silent) { print (DF.sample) }
  return (DF.sample)
}

#=====================================================================
# Renaming in data frames

# Rename a single column of a data frame
rename.col <- function (DF, old, new) {
  stopifnot (is.data.frame (DF))
  names (DF)[names (DF) == old] <- new
  return (DF)
}

#=====================================================================
# Scan a list of data frames, 'DF.list', and return the largest set of
# column names common to all frames.
#
# Note: If 'DF.list' is not a list but a data frame, then this
# function returns the column names; otherwise, it returns NA.

get.common.colnames <- function (Df.list) {
  Common <- NA
  if (is.list (Df.list)) {
    for (D in Df.list) {
      stopifnot (is.data.frame (D))
      if (all (is.na (Common))) { # first one
        Common <- colnames (D)
      } else {
        Common <- intersect (Common, colnames (D))
      }
    } # D
  } else if (is.data.frame (Df.list)) {
    Common <- colnames (Df.list)
  }
  return (Common)
}

#=====================================================================
# Consider a data frame D whose columns are given by the vector of
# names N. Given a subset I of column names, this function splits D
# into two new data frames, A and B, such that A = D[, I] and B = D[,
# N \ I].

split.df.by.colnames <- function (D, I) {
  # Preconditions
  stopifnot (is.data.frame (D))
  if (is.null (I)) { return (D) }
  stopifnot (is.character (I) | is.integer (I))

  N <- colnames (D)
  A <- D[, I]
  B <- D[, setdiff (N, I)]
  return (list (A=A, B=B))
}

#=====================================================================
# "Flatten" certain columns of a data frame into key-value pairs,
# retaining an optional subset of other columns.
#
# More precisely, Let F be a data frame with m rows and any number of
# columns, and let V be another data frame with m rows and n
# columns. Let k[i] be the name ("key") of column V[:,i]. Then, the
# this function returns the following as a data frame:
#
#   F[1,:] k[1] V[1,1]    # begin V[:,1]
#   F[2,:] k[1] V[2,1]
#   ...
#   F[m,:] k[1] V[m,1]    # end V[:,1]
#   F[1,:] k[1] V[1,2]    # begin V[:,2]
#   F[2,:] k[1] V[2,2]
#   ...
#   F[m,:] k[1] V[m,2]    # end V[:,2]
#   ...
#   F[1,:] k[n] V[1,n]    # begin V[:,n]
#   F[2,:] k[n] V[2,n]
#   ...
#   F[m,:] k[n] V[m,n]    # end V[:,n]

flatten.keyvals.df <- function (Fixed=NULL, Values) {
  stopifnot (is.data.frame (Fixed))
  stopifnot (is.data.frame (Values))

  # If Key.cols not specified, fill in default
  Key.cols <- colnames (Values)
  
  New <- NULL
  for (Key in Key.cols) {
    Value <- Values[[Key]]
    New <- rbind (New, cbind (Fixed, Key, Value))
  }
  return (New)
}

#=====================================================================
# Merges a list of data frames on a given set of columns. If the
# column set is NULL, then detect the greatest common subset of
# columns and merge those.

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

#=====================================================================
# Wrapper around 'lm' to perform a regression fit, given a data frame
# and the response and predictor variables given by column
# names. (That is, by column names instead of by a formula, as in
# 'lm'.)

lm.by.colnames <- function (Data.fit, response.var, Predictors, constant.term=TRUE, verbose=TRUE) {
  lhs <- response.var
  rhs.const <- if (constant.term) "" else "0 + "
  rhs.preds <- paste (Predictors, collapse=" + ")
  fit.formula <- as.formula (sprintf ("%s ~ %s%s", lhs, rhs.const, rhs.preds))
  if (verbose) {
    cat ("==> Fitting formula: ")
    print (fit.formula)
    cat ("\n")
  }
  Fit.lm <- lm (fit.formula, data=Data.fit)
  return (Fit.lm)
}

# Given an 'lm' object, return the intercept value
get.intercept.lm <- function (fit.lm) {
  stopifnot (class (fit.lm) %in% "lm")
  if ("(Intercept)" %in% names (fit.lm$coef)) {
    fit.intercept <- fit.lm$coef[["(Intercept)"]]
  } else {
    fit.intercept <- NULL
  }
  return (fit.intercept)
}

# Given an 'lm' object, return the predictor weights (coefficients)
get.predictor.coefs.lm <- function (fit.lm) {
  stopifnot (class (fit.lm) %in% "lm")
  Predictors <- setdiff (names (Fit.lm$coef), "(Intercept)")
  Fit.coefs <- Fit.lm$coef[Predictors]
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
predict.df <- function (fit.lm, Df, response.var="Y") {
  stopifnot (class (fit.lm) %in% "lm")
  stopifnot (is.data.frame (Df))
  
  # Extract fit coefficients for later use. If an intercept is
  # available, extract it as a separate variable (or NULL if not available).
  fit.intercept <- get.intercept.lm (Fit.lm)
  Fit.coefs <- get.predictor.coefs.lm (Fit.lm)

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

#=====================================================================
# Remap factor values and fix the order

re.factor <- function (F, from, to) factor (mapvalues (F, from, to), levels=to)

#=====================================================================
# Given a vector, extract even or odd elements

odd.elements <- function (X) {
  return (X[seq (from=1, to=length (X), by=2)])
}

even.elements <- function (X) {
  return (X[seq (from=2, to=length (X), by=2)])
}

#=====================================================================
# Min / max alternatives

# Returns the minimum value among all positive values of X
min.positive <- function (X) min (X[X > 0])

# Returns the maximum value among all finite values of X
max.finite <- function (X) max (X[is.finite (X)])

#=====================================================================
# Vector versions of which.min, which.max

which.pmin3 <- function (A, B, C, values=c ("A", "B", "C")) {
  Result = mapply (function (a, b, c) values[which.min (c (a, b, c))], A, B, C)
  return (factor (Result, levels=values))
}
which.pmax3 <- function (A, B, C, values=c ("A", "B", "C")) {
  Result = mapply (function (a, b, c) values[which.max (c (a, b, c))], A, B, C)
  return (factor (Result, levels=values))
}

#=====================================================================
# List-compatible versions of arithmetic routines

# 1 / x
lc.reciprocal <- function (X)
  if ("list" %in% class (X)) lapply (X, function (x) 1/x) else (1/X)

unlist.divide <- function (A, B) unlist (A) / unlist (B)

#=====================================================================
# Convert a vector of numbers, X[1:n], into a vector of strings,
# Y[1:n], where each Y[i] is the value X[i] rounded to k significant
# figures.

genSigFigLabels <- function (X, k) {
  sapply (X, function (x) {
    if (is.na (x)) {
      y <- "NA"
    } else if (x < 0 | x > 0) {
      l <- floor (log10 (abs (x)))
      if (k <= l)
        y <- sprintf ("%d", signif (x, k))
      else # k >= l
        y <- sprintf (sprintf ("%%.%df", k-l-1), x)
    } else { # x == 0
#      y <- sprintf (sprintf ("%%.%df", k-1))
      y <- paste (c ("0.", rep ("0", k-1)), collapse="")
    }
    return (y)
  })
}

#======================================================================
# Utility functions to determine what SI units to use

fmt.fun.def <- function (x) sprintf ("%d", round (x))

getUnit.IECbin <- function (x, fmt.fun=fmt.fun.def) {
  if (x >= (2^80)) return (sprintf ("%s Yi", fmt.fun (x/(2^80))))
  if (x >= (2^70)) return (sprintf ("%s Zi", fmt.fun (x/(2^70))))
  if (x >= (2^60)) return (sprintf ("%s Ei", fmt.fun (x/(2^60))))
  if (x >= (2^50)) return (sprintf ("%s Pi", fmt.fun (x/(2^50))))
  if (x >= (2^40)) return (sprintf ("%s Ti", fmt.fun (x/(2^40))))
  if (x >= (2^30)) return (sprintf ("%s Gi", fmt.fun (x/(2^30))))
  if (x >= (2^20)) return (sprintf ("%s Mi", fmt.fun (x/(2^20))))
  if (x >= (2^10)) return (sprintf ("%s Ki", fmt.fun (x/(2^10))))
  return (sprintf ("%s ", fmt.fun (x)))
}

getUnit.SI <- function (x, fmt.fun=fmt.fun.def) {
  if (x >= (10^24)) return (sprintf ("%s Y", fmt.fun (x/(10^24))))
  if (x >= (10^21)) return (sprintf ("%s Z", fmt.fun (x/(10^21))))
  if (x >= (10^18)) return (sprintf ("%s E", fmt.fun (x/(10^18))))
  if (x >= (10^15)) return (sprintf ("%s P", fmt.fun (x/(10^15))))
  if (x >= (10^12)) return (sprintf ("%s T", fmt.fun (x/(10^12))))
  if (x >= (10^9)) return (sprintf ("%s G", fmt.fun (x/(10^9))))
  if (x >= (10^6)) return (sprintf ("%s M", fmt.fun (x/(10^6))))
  if (x >= (10^3)) return (sprintf ("%s K", fmt.fun (x/(10^3))))
  if (x >= (10^0)) return (sprintf ("%s ", fmt.fun (x)))
  if (x >= (10^(-2))) return (sprintf ("%s c", fmt.fun (x/(10^(-2)))))
  if (x >= (10^(-3))) return (sprintf ("%s m", fmt.fun (x/(10^(-3)))))
  if (x >= (10^(-6))) return (sprintf ("%s u", fmt.fun (x/(10^(-6)))))
  if (x >= (10^(-9))) return (sprintf ("%s n", fmt.fun (x/(10^(-9)))))
  if (x >= (10^(-12))) return (sprintf ("%s p", fmt.fun (x/(10^(-12)))))
  if (x >= (10^(-15))) return (sprintf ("%s f", fmt.fun (x/(10^(-15)))))
  return (sprintf ("%s ", fmt.fun (x)))
}

getUnits.IECbin <- function (X, fmt.fun=fmt.fun.def) {
  sapply (X, FUN=function (x) getUnit.IECbin (x, fmt.fun=fmt.fun))
}

getUnits.IECbin.Bytes <- function (X, fmt.fun=fmt.fun.def) {
  sprintf ("%sB", getUnits.IECbin (X, fmt.fun=fmt.fun))
}

getUnits.SI <- function (X, fmt.fun=fmt.fun.def) {
  sapply (X, FUN=function (x) getUnit.SI (x, fmt.fun=fmt.fun))
}

#======================================================================
# Generates reader-friendly SI unit strings

str.SI <- function (x, unit="", sigfigs=3, IECbin=FALSE)
  if (is.na (x)) {
    "NA"
  } else {
    fmt.fun <- if (IECbin) getUnit.IECbin else getUnit.SI
    sprintf ("%s%s"
             , fmt.fun (x, function (x) genSigFigLabels (x, sigfigs))
             , unit)
  }

str.SI.FLOPS <- function (x, sigfigs=3)
  return (str.SI (x, unit="flop/s", sigfigs=sigfigs))

str.SI.spFLOP <- function (x, sigfigs=3)
  return (str.SI (x, unit="s/flop", sigfigs=sigfigs))

str.SI.Bps <- function (x, sigfigs=3, IECbin=FALSE)
  return (str.SI (x, unit="B/s", sigfigs=sigfigs, IECbin=IECbin))

str.SI.spB <- function (x, sigfigs=3, IECbin=FALSE)
  return (str.SI (x, unit="s/B", sigfigs=sigfigs, IECbin=IECbin))

str.SI.AccRate <- function (x, sigfigs=3, IECbin=FALSE)
  return (str.SI (x, unit="acc/s", sigfigs=sigfigs, IECbin=IECbin))

str.SI.FLOPperJ <- function (x, sigfigs=3)
  return (str.SI (x, unit="flop/J", sigfigs=sigfigs))

str.SI.JperFLOP <- function (x, sigfigs=3)
  return (str.SI (x, unit="J/flop", sigfigs=sigfigs))

str.SI.JperB <- function (x, sigfigs=3)
  return (str.SI (x, unit="J/B", sigfigs=sigfigs))

str.SI.BperJ <- function (x, sigfigs=3)
  return (str.SI (x, unit="B/J", sigfigs=sigfigs))

str.SI.Watts <- function (x, sigfigs=3)
  return (str.SI (x, unit="W", sigfigs=sigfigs))

# eof
