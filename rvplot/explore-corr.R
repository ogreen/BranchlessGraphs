#======================================================================
# Use this script to plot and compare instruction mixes, given a
# platform.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

# Load data, if not already available
assign.if.undef ("All.data", load.perfdata.many ())
All.codes <- unique (get.all.colvals (All.data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

if (!BATCH) {
  prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL.MAP)
  prompt.if.undef ("ALG", keyword="algorithms", unique (All.data[[ARCH]]$Algorithm))
  prompt.if.undef ("CODE", keyword="implementations", unique (All.data[[ARCH]]$Implementation))
} else {
  assign.if.undef ("ARCH", "Haswell")
  assign.if.undef ("ALG", "SV")
  assign.if.undef ("CODE", "Branch-based")
}

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot ((length (ALG) == 1) & (ALG %in% ALGS.ALL.MAP))
stopifnot ((length (CODE) == 1) & (CODE %in% CODES.ALL.MAP))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALG, CODE)
outfilename.corr <- sprintf ("figs2/explore-corr--%s.pdf", outfile.suffix)
outfilename.vars <- sprintf ("figs2/explore-corr-vars--%s.csv", outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  Correlations: %s\n", outfilename.corr))
cat (sprintf ("  Analysis variables: %s\n", outfilename.vars))

#======================================================================
# Preprocess data

Df <- get.perfdf.arch (All.data, ARCH, ALG, CODE)
Vars <- get.perfdf.var.info (Df, All.data)

# Normalize by instructions per iteration _of the branchy code_, if available
Inst.norm <- get.perfdf.norm (Df, Vars, by="Instructions")
Df.per.inst <- normalize.perfdf (Df, Vars, Inst.norm)

# Define an initial list of variables to consider for analysis
Init.vars <- c (if (Vars$has.cycles) Vars$Platform else "Time", "Branches", "Mispredictions")

# Don't consider any variable that is *all* zero
Is.all.zero <- colwise (function (X) all (X == 0)) (Df.per.inst[, Init.vars])
Valid.vars <- names (Is.all.zero)[unlist (!Is.all.zero)]

# Visualize correlations
Q.corr <- ggpairs (Df.per.inst, Valid.vars, upper=list (continuous="points", combo="dot"), lower=list (continuous="cor"))
if (!BATCH) {
  do.corr <- prompt.yes.no ("\nPlot pairwise corelations? (may be slow) ")
  if (do.corr) {
    setDevSquare ()
    print (Q.corr)
  }
  do.corr.pdf <- prompt.yes.no (sprintf ("\nSave to '%s'? ", outfilename.corr))
} else {
  do.corr.pdf <- SAVE.PDF
}

if (do.corr.pdf) {
  cat (sprintf ("\nWriting to '%s' ...", outfilename.corr))
  setDevSquare.pdf (outfilename.corr, l=18)
  print (Q.corr)
  dev.off ()
  cat (sprintf (" done!\n\n"))
}

# Prompt user to select a subset of variables to further consider modeling
cat ("\n=== Correlations ===\n")
Rho <- cor (Df.per.inst[, Valid.vars])
print (Rho)

if (!BATCH) {
  cat ("\n*** Inspect the above correlations and refer to the pairwise correlations plot. Decide which variables you'd like to consider.\n")

  response.var <- if (Vars$has.cycles) "Cycles" else "Time" # Always consider
  Avail.vars <- setdiff (Valid.vars, response.var)
  Selected.vars <- prompt.select.any (Avail.vars, keyword="variables TO ELIMINATE")
  if (is.null (Selected.vars)) {
    Analysis.vars <- Avail.vars
  } else {
    Analysis.vars <- setdiff (Avail.vars, Selected.vars)
  }
  cat (sprintf ("\n==> Final set of analysis variables: %s\n"
                , paste (Analysis.vars, collapse=", ")))

  # TODO: Add code to save these to a file
  do.save.vars <- prompt.yes.no (sprintf ("Save to '%s'? ", outfilename.vars))
  if (do.save.vars) {
    cat ("\nWriting ...")
    write (Analysis.vars, file=outfilename.vars, sep="\n")
    cat (" done!\n\n")
  }
}

#======================================================================
# eof
