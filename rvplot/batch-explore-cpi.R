source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE

for (ARCH in ARCHS.ALL.MAP) {
  rm (list=c ("ALGS", "CODES"))
  source ("explore-cpi.R")
}

# eof
