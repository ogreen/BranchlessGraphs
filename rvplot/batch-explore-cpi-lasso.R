source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE
FIT.PER.GRAPH <- FALSE
for (ARCH in as.vector (ARCHS.ALL.MAP)) {
  if ("ALGS" %in% ls ()) { rm ("ALGS") }
  if ("CODES" %in% ls ()) { rm ("CODES") }
  source ("explore-cpi-lasso.R")
}

# eof
