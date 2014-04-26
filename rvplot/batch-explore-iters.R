source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE
for (CUMULATIVE in c (FALSE, TRUE)) {
  for (ALG in as.vector (unlist (ALGS.ALL.MAP))) {
    for (METRIC in c ("Time", "Branches", "Mispredictions")) {
      source ("explore-iters.R")
    }
  }
}

# eof
