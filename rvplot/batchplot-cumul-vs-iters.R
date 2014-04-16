source ("rvplot-inc.R")

SAVE.PDF <- TRUE
for (ALG in ALGS) {
  for (ARCH in ARCHS) {
    for (METRIC in c ("Time", "Mispredictions", "Branches", "Instructions")) {
      source ("plot-cumul-vs-iters.R")
    }
  }
}

# eof
