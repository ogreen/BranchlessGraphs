SAVE.PDF <- TRUE
for (COMP in c ("sv", "bfs")) {
  for (ALG in c ("Branch-based", "Branch-avoiding")) {
    for (LOAD.STORE in c (FALSE, TRUE)) {
      source ("plot-corr.R")
    }
  }
}

# eof
