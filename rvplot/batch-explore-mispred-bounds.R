source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE
for (ALG in as.vector (unlist (ALGS.ALL.MAP))) {
  source ("explore-mispred-bounds.R")
}

# eof
