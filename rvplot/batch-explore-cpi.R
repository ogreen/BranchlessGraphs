source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE

for (FIT.PER.GRAPH in c (FALSE, TRUE)) {
  #for (ARCH in ARCHS.ALL.MAP) {
  for (ARCH in c ("Haswell", "Ivy Bridge")) {
    if ("ALGS" %in% ls ()) { rm ("ALGS") }
    if ("CODES" %in% ls ()) { rm ("CODES") }
    source ("explore-cpi.R")
  }
}

# eof
