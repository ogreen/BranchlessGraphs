#library (gsubfn)
library (ggplot2)
library (plyr)
library (directlabels)
# For directlabels:
#library (quadprog)
#library (alphahull)

#TERM.WIDTH <- as.numeric (system ("tput cols", intern=TRUE)) - 1 # "-1" guard
#options (width=TERM.WIDTH)

OS.NAME <- system2 ("uname", stdout=TRUE)
HOST.NAME <- system2 ("hostname", c ("-s"), stdout=TRUE)

if (OS.NAME == "Darwin") options (device=quartz)

HUGE.SCREEN <- (HOST.NAME == "insomnia")

# In inches along the diagonal
SCREEN.SIZE <-
  if (HOST.NAME == "insomnia") {
    27
  } else if (HOST.NAME == "strada") {
    24
  } else if (HOST.NAME == "daffy3" | HOST.NAME == "daffy4") {
    11
  } else {
    15
  }

DEF.SIZE.SLIDE <- ceiling ((1 / 2) * SCREEN.SIZE)
DEF.SIZE.HD <- ceiling ((16 / 24) * SCREEN.SIZE)
DEF.SIZE.SQUARE <- SCREEN.SIZE * 2 / 5
DEF.SIZE.PAGE <- SCREEN.SIZE / 3

#======================================================================

gen_ticks <- function (X, fun_tostep, fun_fromstep, xmin=NULL, xmax=NULL, stepjump=1) {
  first_step <- if (is.null (xmin)) floor (fun_tostep (min (X))) else fun_tostep (xmin)
  max_step <- if (is.null (xmax)) ceiling (fun_tostep (max (X))) else fun_tostep (xmax)
  fun_fromstep (seq (from=first_step, to=max_step, by=stepjump))
}

gen_ticks_linear <- function (X, stepsize, ...) {
  gen_ticks (X, fun_tostep=function (x) x / stepsize, fun_fromstep=function (x) x * stepsize, ...)
}

gen_ticks_log2 <- function (X, ...) {
  gen_ticks (X, fun_tostep=function (x) log2 (x), fun_fromstep=function (x) 2^x, ...)
}

gen_ticks_log10 <- function (X, ...) {
  gen_ticks (X, fun_tostep=function (x) log10 (x), fun_fromstep=function (x) 10^x, ...)
}

add_scale <- function (f_scale, name, ticks, ...) {
  f_scale (name=name, limits=c (min (ticks), max (ticks)), breaks=ticks, labels=ticks, ...)
}

dev.off.safe <- function () {
  if (dev.cur () > 1) dev.off ()
}

setDev <- function (w, h) {
  dev.off.safe ()
  dev.new (width=w, height=h)
}

setDevNarrow <- function (l=6) {
  dev.off.safe ()
  dev.new (width=l * 2 / 3, height=l / 6 * 8 * 2 / 3)
}

setDevSquare <- function (l=DEF.SIZE.SQUARE) {
  dev.off.safe ()
  dev.new (width=l, height=l)
}

setDevSquare.pdf <- function (file, l=6) {
  pdf (file, width=l, height=l)
}

setDevSlide <- function (l=DEF.SIZE.SLIDE) {
  dev.off.safe ()
  dev.new (width=l, height=l/8*6) # Use these dimensions for slides
}

setDevSlide.pdf <- function (file, l=DEF.SIZE.SLIDE) {
  pdf (file, width=l, height=l/8*6) # Use these dimensions for slides
}

setDevHD <- function (l=DEF.SIZE.HD) {
  dev.off.safe ()
  dev.new (width=l, height=l/16*9)  # Use these for HD format (for papers or HD slides)
}

setDevHD.pdf <- function (file, l=8) {
  pdf (file, width=l, height=l/16*9)
}
  
setDevPage.portrait <- function (l=DEF.SIZE.PAGE) {
  dev.off.safe ()
  dev.new (width=l, height=l/6.5*9)
}

setDevPage.portrait.pdf <- function (file, l=DEF.SIZE.PAGE) {
  pdf (file, width=l, height=l/6.5*9)
}

setDevPage.landscape <- function (l=DEF.SIZE.PAGE) {
  dev.off.safe ()
  dev.new (width=l/6.5*9, height=l)
}

setDevPage.landscape.pdf <- function (l=DEF.SIZE.PAGE) {
  pdf (file, width=l/6.5*9, height=l)
}


#======================================================================

genlabels.log2 <- function (X)
  sapply (X, function (x) {
    k <- log2 (abs (x))
    if (x != 0) {
      if (k >= 0) {
        if (k >= 20) {
          return (parse (text=sprintf ("2^%d", k)))
        } else {
          return (sprintf ("%d", x))
        }
      } else {
        s = if (x < 0) "-" else ""
        if (k <= -10) {
          return (parse (text=sprintf ("2^%d", k)))
        } else {
          return (sprintf ("%s1/%d", s, 2^(-log2 (abs (x)))))
        }
      }
    } else {
      return ("0")
    }
  })

#======================================================================

# Needed for ggplot2 >= 0.9.0
Packages = installed.packages ()
if (Packages["ggplot2", "Version"] >= "0.9.0") {
  library (scales)
  
  reverselog_trans <- function(base = exp(1)) {
    trans <- function(x) -log(x, base)
    inv <- function(x) base^(-x)
    trans_new (paste0 ("reverselog-", format (base)), trans, inv, 
              log_breaks (base = base), 
              domain = c (1e-100, Inf))
  }
  
  scale_x_log2 <- function (reverse=FALSE, ...) {
    scale_x_continuous (..., trans=if (reverse) reverselog_trans () else log2_trans ())
  }
  scale_y_log2 <- function (reverse=FALSE, ...) {
    scale_y_continuous (..., trans=if (reverse) reverselog_trans () else log2_trans ())
  }
}

#======================================================================
gen.stepsize.auto <- function (X, numint=10) {
  # Determine which of the following step sizes yields closest to 'numint' intervals
  S.options = c (0.1, 0.2, 0.25, 0.5, 1)

  scale.s <- function (Y, s) {
    w = diff (range (Y)) # width of the interval
    return (s * 10^floor (log10 (w)))
  }
  lims.s <- function (Y, s) {
    s.scaled = scale.s (Y, s)
    L = c (floor (min (Y) / s.scaled)*s.scaled, ceiling (max (Y) / s.scaled)*s.scaled)
    return (L)
  }
  eval.s <- function (Y, s)
    return (diff (range (lims.s (Y, s))) / scale.s (Y, s))

  S.final = c ()
  for (s in S.options)
    S.final = c (S.final, eval.s (X, s))
  s.best = S.options[which.min (abs (S.final - numint))]
  return (list (base=s.best, scaled=scale.s (X, s.best)))
}
    
# Generate an auto-scaled axis (linear or log2)
gen.axis.scale.auto <- function (Values, axis, scale="linear", free=FALSE) {
  stopifnot (scale %in% c ("linear", "log2"))

  if (scale == "linear") {
    scale.func <- if (axis == "x") scale_x_continuous else scale_y_continuous
    if (!free) {
      step <- gen.stepsize.auto (Values)$scaled
      Breaks <- gen_ticks_linear (Values, step)
      return (scale.func (breaks=Breaks, limits=range (Breaks)))
    } else {
      return (scale.func ())
    }
  }
  if (scale == "log2") {
    scale.func <- if (axis == "x") scale_x_log2 else scale_y_log2
    if (!free) {
      Breaks <- gen_ticks_log2 (Values)
      Labels <- genlabels.log2 (Breaks)
      return (scale.func (breaks=Breaks, limits=range (Breaks), labels=Labels))
    } else {
      return (scale.func ())
    }
  }
  
  return (NA)
}

# Add title with optional subtitle.
# Set 'func' to function to invoke, e.g., ggtitle, xlab, ...
# Adapted from: http://www.antoni.fr/blog/?p=39
add.title.optsub <- function (Q, func, main, sub=NA) {
  if (is.na (sub)) {
    Q <- Q + func (eval (parse (text=paste ("expression(atop(\"", main, "\",", "))", sep=""))))
  } else {
    Q <- Q + func (eval (parse (text=paste ("expression(atop(\"", main, "\",", " atop(\"", sub, "\",\"\")))", sep=""))))
  }
  return (Q)
}

#======================================================================
# Applies the HPC Garage colour scheme

# Ocean Five palette:
#    hex rgb: #664b3b (brown), #3f9fb1 (aqua), #c03040 (red), #df693e (orange), #e7ce3f (yellow), #a69e9e (grey)
PAL.HPCGARAGE = c (
  "red"="#c03040"
  , "blue"="#3f9fb1"
  , "brown"="#664b3b"
  , "grey"="#a69e9e"
  , "orange"="#df693e"
  , "yellow"="#e7ce3f"
  , "offwhite"="#d0d0d0"
  , "black"="#000000"
  , "white"="#ffffff"
  )

set.hpcgarage.colours <- function (Q, ...) {
  Q <- Q + scale_colour_manual (values=as.vector (PAL.HPCGARAGE), ...)
  return (Q)
}

set.hpcgarage.fill <- function (Q, ...) {
  Q <- Q + scale_fill_manual (values=as.vector (PAL.HPCGARAGE), ...)
  return (Q)
}

FONT.SIZE.TINY = rel (1.75)
FONT.SIZE.FOOTNOTE = rel (2.25)
FONT.SIZE.SMALL = rel (3.5)
FONT.SIZE.MEDIUM = rel (4)
FONT.SIZE.LARGE = rel (4.5)
FONT.SIZE.VERYLARGE = rel (4.75)
FONT.SIZE.HUGE = rel (5.5)
FONT.SIZE.VERYHUGE = rel (6)

#======================================================================
# eof
