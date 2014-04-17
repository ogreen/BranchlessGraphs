SAVE.PDF <- TRUE
for (COMP in c ("sv", "bfs")) {
  for (ALG in c ("Branch-based", "Branch-avoiding")) {
    source ("plot-corr.R")
  }
}

# eof
