#=====================================================================
# Assigns a variable a given value, but *only* if the variable doesn't
# already exist; otherwise, leaves the variable's value intact.

assign.if.undef <- function (var.name, value.if.new, env=parent.frame (), ...)
  if (!(var.name %in% ls (envir=env))) assign (var.name, value.if.new, envir=env, ...)

#=====================================================================
# User prompts

# If the string s is in the set of strings S, returns the
# corresponding element(s) of S. Otherwise, returns the empty string.
get.stringset.elem <- function (S, s, ignore.case=FALSE) {
  if (is.null (S)) { return ("") } # S is empty or invalid
  if (all (is.na (S))) { return ("") } # S is empty of invalid
  if (ignore.case) {
    Matches <- tolower (S) %in% tolower (s)
  } else {
    Matches <- S %in% s
  }
  if (!any (Matches)) { return ("") }
  return (if (length (S[Matches]) > 1) S[Matches][1] else S[Matches])
}

prompt.yes.no <- function (prompt.text="Type yes or type no >>> ") {
  result <- NULL
  while (is.null (result)) {
    user.text <- tolower (readline (prompt.text))
    if (any (grepl ("yes|no", user.text, ignore.case=TRUE))) {
      result <- (user.text == "yes")
      break
    }
    cat (sprintf ("\n*** Did not recognize: '%s' ***\n\n", user.text))
  }
  cat ("\n")
  return (result)
}

prompt.select.string <- function (Options, keyword="options"
                                  , caption=NULL
                                  , is.empty.ok=FALSE
                                  , ignore.case=TRUE
                                  , Silent.options=NULL)
{
  opt <- NULL
  while (is.null (opt)) {
    cat (sprintf ("Select one of the following %s:\n  %s\n", keyword, paste (Options, collapse=", ")))
    if (!is.null (caption)) { cat (sprintf ("\n(%s)\n", caption)) }
    user.text <- readline ("\n>>> ")

    # Break "quietly" on silent option, returning value of silent option
    silent.text <- get.stringset.elem (Silent.options, user.text, ignore.case=ignore.case)
    if (silent.text != "") { opt <- silent.text ; break }

    # Break with chosen option
    opt.text <- get.stringset.elem (Options, user.text, ignore.case=ignore.case)
    if (!is.null (opt.text)) {
      if (opt.text != "") {
        opt <- opt.text
        break
      }
    }

    # Did user just press enter by itself?
    if (user.text == "") {
      if (is.empty.ok) {
        opt <- user.text
        break
      }
      next # blank response not ok, so keep asking
    }

    # Don't know what the user wants (and maybe neither does s/he)
    cat (sprintf ("*** Sorry, '%s' is not recognized. ***\n", user.text))
  }
  cat ("\n")
  if (!is.null (opt)) { opt <- as.character (opt) }
  return (opt)
}

pause.for.enter <- function (ignore=FALSE) {
  if (!ignore) {
    readline ("\n=== Press <Enter> to continue ===\n")
  }
}

prompt.select.any <- function (Options, keyword="options"
                               , confirm=TRUE, ignore.case=TRUE)
{
  Selected <- NULL
  while (TRUE) {
    cat ("Please select one or more of the following", keyword, "\n")
    cat (sprintf ("  %s\n", paste (Options, collapse=", ")))
    cat ("\n(You may use a regular expression.)\n")
    user.text <- readline ("\n>>> ")
    if (user.text != "") {
      Matched <- grepl (user.text, Options, ignore.case=ignore.case)
      if (any (Matched)) {
        Selected <- Options[Matched]
      }
    }

    if (is.null (Selected)) {
      cat ("\n--- Nothing selected or no match found. ---\n")
    } else {
      cat (sprintf ("\n--- Selected: '%s' ---\n", paste (Selected, collapse=", ")))
    }
    if (confirm) {
      do.confirm <- prompt.yes.no ("\nAre you sure? ")
      if (!do.confirm) { next } # User changed his/her mind
    }
    break
  }
  cat ("\n")
  if (!is.null (Selected)) { Selected <- as.character (Selected) }
  return (Selected)
}

prompt.if.undef <- function (var.name, ..., env=parent.frame ()) {
  if (!(var.name %in% ls (envir=env))) {
    assign (var.name, prompt.select.string (...), envir=env)
  }
}

prompt.any.if.undef <- function (var.name, ..., env=parent.frame ()) {
  if (!(var.name %in% ls (envir=env))) {
    assign (var.name, prompt.select.any (...), envir=env)
  }
}

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
# Scan a list of data frames, 'DF.list', and return the union of all
# column names.
#
# Note: If 'DF.list' is not a list but a data frame, then this
# function returns the column names; otherwise, it returns NA.

get.all.colnames <- function (Df.list) {
  All <- NA
  if (is.list (Df.list)) {
    for (D in Df.list) {
      stopifnot (is.data.frame (D))
      if (all (is.na (All))) { # first one
        All <- colnames (D)
      } else {
        All <- unique (c (All, colnames (D)))
      }
    } # D
  } else if (is.data.frame (Df.list)) {
    All <- colnames (Df.list)
  }
  return (as.vector (unlist (All)))
}

#=====================================================================
# Scan a list of data frames and collect all values of a given column,
# when it exists.

get.all.colvals <- function (Df.list, col) {
  All <- NULL
  if (is.list (Df.list)) {
    for (D in Df.list) {
      stopifnot (is.data.frame (D))
      if (col %in% colnames (D)) {
        All <- rbind (All, D[col])
      }
    }
  } else if (is.data.frame (Df.list)) {
    if (col %in% colnames (D)) {
      All <- rbind (All, D[col])
    }
  }
  return (as.vector (unlist (All)))
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
# rbind that fills in missing columns

rbind.fill <- function (A, B, missing.val=0) {
  if (is.null (A)) return (B)
  if (is.null (B)) return (A)
  
  stopifnot (is.data.frame (A))
  stopifnot (is.data.frame (B))

  cols.A <- colnames (A)
  cols.B <- colnames (B)
  cols.All <- unique (c (cols.A, cols.B))

  missing.A <- setdiff (cols.All, cols.A)
  if (length (missing.A) > 0) {
    A[, missing.A] <- 0
  }
  missing.B <- setdiff (cols.All, cols.B)
  if (length (missing.B) > 0) {
    B[, missing.B] <- 0
  }

  C <- rbind (A, B)
  return (C)
}

#=====================================================================
# Computes the cumulative sum

cumsum.df.select <- function (Df, Fixed, Vars, preserve.cols=TRUE) {
  stopifnot (is.data.frame (Df))
  f.apply <- function (X) ddply (Df, Fixed, transform, X=cumsum (X))
  Df.cumul <- colwise (f.apply, Vars) (Df)
  if (preserve.cols) {
    Vars.left <- setdiff (colnames (Df), c (Fixed, Vars))
    Df.cumul[, Vars.left] <- Df[, Vars.left]
  }
  return (Df.cumul)
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
