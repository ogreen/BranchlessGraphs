SAVE.PDF <- TRUE
LOAD.STORE <- FALSE
for (COMP in c ("sv", "bfs")) {
  for (ALG in c ("Branch-based", "Branch-avoiding")) {
    source ("plot-corr.R")
  }
}

LOAD.STORE <- TRUE
for (COMP in c ("sv", "bfs")) {
  for (ALG in c ("Branch-based", "Branch-avoiding")) {
    source ("plot-corr.R")
  }
}

# eof
