#=====================================================================
# Assigns a variable a given value, but *only* if the variable doesn't
# already exist; otherwise, leaves the variable's value intact.

assign.if.undef <- function (var.name, value.if.new, env=parent.frame (), ...)
  if (!(var.name %in% ls (envir=env))) assign (var.name, value.if.new, envir=env, ...)

#=====================================================================
# Halt if the condition is true

stopif <- function (cond) stopifnot (!cond)

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
