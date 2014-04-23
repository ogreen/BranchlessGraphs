source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE
for (ARCH in as.vector (unlist (ARCHS.ALL.MAP))) {
  source ("explore-imix.R")
}

# eof
