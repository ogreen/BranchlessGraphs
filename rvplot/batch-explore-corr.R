source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE

for (ARCH in ARCHS.ALL.MAP) {
  for (ALG in ALGS.ALL.MAP) {
    for (CODE in CODES.ALL.MAP) {
      source ("explore-corr.R")
    }
  }
}

# eof
